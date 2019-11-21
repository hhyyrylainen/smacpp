#!/usr/bin/env ruby
# Runs all the tests for seeing how smacpp compares to clang analyzer and also
# if they were both used how good the results were
require 'open3'
require 'json'
require 'fileutils'
# require 'nokogiri'

SMACPP_PATH = 'build/src/smacpp'.freeze

FileUtils.mkdir_p 'temp'

KEEP_FRAMA_OUTPUT = true

def run_process(*args)
  Dir.chdir 'temp' do
    # puts "command: #{args.join ' '}"
    starting = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    stdout, stderr, status = Open3.capture3(*args)
    ending = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    [stdout, stderr, status.exitstatus, ending - starting]
  end
end

def single_clang_run(compiler_options, file)
  stdout, stderr, status, time = run_process(
    'clang', '--analyze', *compiler_options, file
  )

  error_count = 0
  warning_count = 0

  if (match = stderr.match(/(\d+)\s+errors?\sgenerated/i))
    error_count = match.captures[0].to_i
  end
  if (match = stderr.match(/(\d+)\s+warnings?\sgenerated/i))
    warning_count = match.captures[0].to_i
  end

  {
    out: stdout,
    stderr: stderr,
    exit_code: status,
    error_count: error_count,
    warning_count: warning_count,
    time: time
  }
end

def single_smacpp_run(compiler_options, file)
  raise 'smacpp analyzer plugin missing' unless File.exist? SMACPP_PATH

  stdout, stderr, status, time = run_process File.realpath(SMACPP_PATH),
                                             *compiler_options, file
  # , '-o', '/dev/null'

  error_count = 0
  warning_count = 0

  if (match = stderr.match(/(\d+)\s+errors?\sgenerated/i))
    error_count = match.captures[0].to_i
  end
  if (match = stderr.match(/(\d+)\s+warnings?\sgenerated/i))
    warning_count = match.captures[0].to_i
  end

  {
    out: stdout,
    stderr: stderr,
    exit_code: status,
    error_count: error_count,
    warning_count: warning_count,
    time: time
  }
end

def single_frama_run(compiler_options, file)
  stdout, stderr, status, time = run_process 'frama-c',
                                             '-eva',
                                             '-cpp-extra-args=' +
                                             compiler_options.join(' '), file
  error_count = 0
  warning_count = 0

  if (match =
        stdout.match(
          /Frama-C\s+kernel:\s+(\d+)\s+errors?\s+(\d+)\s+warnings?/i
        ))
    error_count = match.captures[0].to_i
    warning_count = match.captures[1].to_i
  end

  {
    out: KEEP_FRAMA_OUTPUT ? stdout : 'removed',
    stderr: KEEP_FRAMA_OUTPUT ? stderr : 'removed',
    exit_code: status,
    error_count: error_count,
    warning_count: warning_count,
    time: time
  }
end

def warnings_or_errors?(run_result)
  run_result[:warning_count] > 0 || run_result[:error_count] > 0
end

def extra_opts_for_tool(compiler_options, tool)
  # Ignore some warnings
  if tool == :clang || tool == :smacpp
    unless compiler_options.include? '-Wno-return-type'
      compiler_options.append '-Wno-return-type'
    end
  end

  # Ignore linking fails
  if tool == :smacpp
    unless compiler_options.include? '-fsyntax-only'
      compiler_options.append '-fsyntax-only'
    end
  end
end

def check_result(correct_runs, incorrect_runs)
  time = (correct_runs + incorrect_runs).map { |i| i[:time] }.sum
  if correct_runs.any? { |run| warnings_or_errors? run }
    [false, 'false positive', time]
  elsif incorrect_runs.any? { |run| !warnings_or_errors? run }
    [false, 'false negative', time]
  else
    [true, '', time]
  end
end

def run_test_cases_for_single_tool(results, tool, compiler_options, correct,
                                   catch_bad, incorrect)
  if tool == :clang
    extra_opts_for_tool compiler_options, tool

    correct_result = single_clang_run(compiler_options, correct)
    catch_bad_result = single_clang_run(compiler_options, catch_bad)
    incorrect_result = single_clang_run(compiler_options, incorrect)

    success, failure_type, time = check_result(
      [correct_result, catch_bad_result],
      [incorrect_result]
    )

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: time,
      raw: {
        correct_result: correct_result,
        catch_bad_result: catch_bad_result,
        incorrect_result: incorrect_result
      }
    }
  elsif tool == :smacpp
    extra_opts_for_tool compiler_options, tool

    correct_result = single_smacpp_run(compiler_options, correct)
    catch_bad_result = single_smacpp_run(compiler_options, catch_bad)
    incorrect_result = single_smacpp_run(compiler_options, incorrect)

    success, failure_type, time = check_result(
      [correct_result, catch_bad_result],
      [incorrect_result]
    )

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: time,
      raw: {
        correct_result: correct_result,
        catch_bad_result: catch_bad_result,
        incorrect_result: incorrect_result
      }
    }
  elsif tool == :frama
    correct_result = single_frama_run(compiler_options, correct)
    catch_bad_result = single_frama_run(compiler_options, catch_bad)
    incorrect_result = single_frama_run(compiler_options, incorrect)

    success, failure_type, time = check_result(
      [correct_result, catch_bad_result],
      [incorrect_result]
    )
    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: time,
      raw: {
        correct_result: correct_result,
        catch_bad_result: catch_bad_result,
        incorrect_result: incorrect_result
      }
    }
  else
    raise ArgumentError, 'invalid tool'
  end
end

def run_test_cases_single_flags(results, tool, good_flags, incorrect_flags, file)
  if tool == :clang
    extra_opts_for_tool good_flags, tool
    extra_opts_for_tool incorrect_flags, tool

    correct_result = single_clang_run(good_flags, file)
    incorrect_result = single_clang_run(incorrect_flags, file)

    success, failure_type, time = check_result(
      [correct_result],
      [incorrect_result]
    )

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: time,
      raw: {
        correct_result: correct_result,
        incorrect_result: incorrect_result
      }
    }
  elsif tool == :smacpp
    extra_opts_for_tool good_flags, tool
    extra_opts_for_tool incorrect_flags, tool

    correct_result = single_smacpp_run(good_flags, file)
    incorrect_result = single_smacpp_run(incorrect_flags, file)

    success, failure_type, time = check_result(
      [correct_result],
      [incorrect_result]
    )

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: time,
      raw: {
        correct_result: correct_result,
        incorrect_result: incorrect_result
      }
    }
  elsif tool == :frama
    correct_result = single_frama_run(good_flags, file)
    incorrect_result = single_frama_run(incorrect_flags, file)

    success, failure_type, time = check_result(
      [correct_result],
      [incorrect_result]
    )

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: time,
      raw: {
        correct_result: correct_result,
        incorrect_result: incorrect_result
      }
    }
  else
    raise ArgumentError, 'invalid tool'
  end
end

def run_test_cases(results, includes, correct, catch_bad, incorrect)
  raise ArgumentError, "missing file: #{correct}" unless File.exist? correct
  raise ArgumentError, "missing file: #{catch_bad}" unless File.exist? catch_bad
  raise ArgumentError, "missing file: #{incorrect}" unless File.exist? incorrect

  run_test_cases_for_single_tool results, :clang,
                                 includes.map { |i| ['-I', i] }.flatten,
                                 correct, catch_bad, incorrect
  run_test_cases_for_single_tool results, :smacpp,
                                 includes.map { |i| ['-I', i] }.flatten,
                                 correct, catch_bad, incorrect
  run_test_cases_for_single_tool results, :frama,
                                 includes.map { |i| ['-I', i] }.flatten,
                                 correct, catch_bad, incorrect
end

def run_test_cases_defines(results, good_flags, incorrect_flags, file)
  run_test_cases_single_flags results, :clang,
                              good_flags, incorrect_flags, file
  run_test_cases_single_flags results, :smacpp,
                              good_flags, incorrect_flags, file
  run_test_cases_single_flags results, :frama,
                              good_flags, incorrect_flags, file
end

def handle_jm_case(includes, include_folder, file)
  result = {}

  run_test_cases result, includes,
                 File.join(include_folder, 'test_correct', file),
                 File.join(include_folder, 'test_correct_catch_bad', file),
                 File.join(include_folder, 'test_incorrect', file)

  result
end

def handle_juliet_case(includes, file)
  result = {}

  flags = includes.map { |i| ['-I', i] }.flatten + ['-DINCLUDEMAIN']

  run_test_cases_defines result, flags + ['-DOMITBAD'],
                         flags + ['-DOMITGOOD'], file

  result
end

def jm2018ts_test_case(results, folder)
  include_folder = File.absolute_path folder

  name_prefix = (folder.split('/').last 2).join '/'

  includes = [include_folder, File.absolute_path('test/data/JM2018TS/common')]

  Dir.entries(folder).each do |f|
    full_path = File.join include_folder, f

    next if File.directory? full_path

    next unless f =~ /\d\d_.*/i

    case_name = name_prefix + '/' + f
    puts "Running test case: #{case_name}"
    results[case_name] = handle_jm_case includes, include_folder, f
  end
end

def juliet_test_cases(results, folder)
  folder = File.absolute_path folder
  makefile = File.join folder, 'Makefile'
  raise ArgumentError, 'Juliet test case folder has no Makefile' unless
    File.exist? makefile

  includes = [File
    .absolute_path(
      'test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C/testcasesupport'
    )]

  Dir.entries(folder).each do |f|
    full_path = File.join folder, f

    next if File.directory? full_path

    next unless f =~ /CWE.*\.c/i

    puts "Running test case: #{f}"
    results[f] = handle_juliet_case includes, full_path
  end
end

def get_failure_reasons(results, tool)
  results.select { |_key, value| value[tool][:success] == false }
         .map { |_key, value| value[tool] }
         .group_by do |value|
    value[:failure_type]
  end.map do |key, values|
    {
      type: key,
      count: values.length
    }
  end
end

def generate_summary(results, name)
  failure_reasons = {
    clang: get_failure_reasons(results, :clang),
    smacpp: get_failure_reasons(results, :smacpp),
    frama: get_failure_reasons(results, :frama)
  }

  summary = {
    total_runtimes: {
      clang: results.map { |_key, value| value[:clang][:time] }.sum,
      smacpp: results.map { |_key, value| value[:smacpp][:time] }.sum,
      frama: results.map { |_key, value| value[:frama][:time] }.sum
    },
    total_tests: results.length,
    passed: {
      clang: results.select { |_key, value| value[:clang][:success] }.length,
      smacpp: results.select { |_key, value| value[:smacpp][:success] }.length,
      frama: results.select { |_key, value| value[:frama][:success] }.length
    },
    failure_reasons: failure_reasons
  }

  File.write "#{name}_results_summary.json", JSON.pretty_generate(summary)
end

def write_results(results, name)
  File.write "#{name}_results.json", JSON.pretty_generate(results)

  File.open("#{name}_results_table.tex", 'w') do |file|
    results.sort_by { |_k, value| value[:smacpp][:success] ? 0 : 1 }
           .each do |key, value|
      smacpp = value[:smacpp][:success]
      clang_success = value[:clang][:success]
      frama_success = value[:frama][:success]
      sanitized_key = key.gsub('_', '\_').tr('/', ' ')

      # failure = value[:clang][:failure_type] unless clang_success

      # combined_result = clang_success || smacpp

      failure = value[:smacpp][:failure_type]

      file.puts "#{sanitized_key} & #{clang_success} & #{frama_success} & " \
                "#{smacpp} & #{failure} \\\\"
      file.puts('\hline')
    end
  end
end

def run_jm2018ts
  results = {}
  jm2018ts_test_case results, 'test/data/JM2018TS/strings/unbounded_copy'
  jm2018ts_test_case results, 'test/data/JM2018TS/strings/overflow'
  jm2018ts_test_case results, 'test/data/JM2018TS/memory/double_free'
  jm2018ts_test_case results, 'test/data/JM2018TS/memory/access_uninit'
  jm2018ts_test_case results, 'test/data/JM2018TS/memory/leak'
  jm2018ts_test_case results, 'test/data/JM2018TS/memory/refer_free'
  jm2018ts_test_case results, 'test/data/JM2018TS/memory/zero_alloc'

  results
end

def run_juliet_cases
  results = {}
  juliet_test_cases(
    results,
    'test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C/' \
    'testcases/CWE126_Buffer_Overread/s01'
  )

  juliet_test_cases(
    results,
    'test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C/' \
    'testcases/CWE126_Buffer_Overread/s02'
  )

  # has no makefile so supposedly these are all windows-only
  # juliet_test_cases(
  #   'test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C/' \
  #   'testcases/CWE126_Buffer_Overread/s03'
  # )

  results
end

def do_jm_run
  puts 'Running jm (Moerman) cases'
  results = run_jm2018ts

  write_results results, 'jm'
  generate_summary results, 'jm'
end

def do_juliet_run
  puts 'Running juliet cases'
  results = run_juliet_cases

  write_results results, 'juliet'
  generate_summary results, 'juliet'
end

do_jm_run

do_juliet_run

puts ''
puts 'Finished running'
