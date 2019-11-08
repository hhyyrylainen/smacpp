#!/usr/bin/env ruby
# Runs all the tests for seeing how smacpp compares to clang analyzer and also if
# they were both used how good the results were
require 'open3'
require 'json'
require 'fileutils'

$results = {}

SMACPP_PATH = 'build/src/smacpp'.freeze

FileUtils.mkdir_p 'temp'

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
    out: stdout,
    stderr: stderr,
    exit_code: status,
    error_count: error_count,
    warning_count: warning_count,
    time: time
  }
end

def warnings_or_errors?(run_result)
  run_result[:warning_count] > 0 || run_result[:error_count] > 0
end

def run_test_cases_for_single_tool(results, tool, compiler_options, correct,
                                   catch_bad, incorrect)
  if tool == :clang

    # Ignore some warnings
    compiler_options.append '-Wno-return-type'

    success = true
    failure_type = ''

    correct_result = single_clang_run(compiler_options, correct)
    catch_bad_result = single_clang_run(compiler_options, catch_bad)
    incorrect_result = single_clang_run(compiler_options, incorrect)

    if warnings_or_errors?(correct_result) ||
       warnings_or_errors?(catch_bad_result)
      success = false
      failure_type = 'false positive'
    elsif !warnings_or_errors? incorrect_result
      success = false
      failure_type = 'false negative'
    end

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: (correct_result[:time] + catch_bad_result[:time] +
             incorrect_result[:time]),
      raw: {
        correct_result: correct_result,
        catch_bad_result: catch_bad_result,
        incorrect_result: incorrect_result
      }
    }
  elsif tool == :smacpp
    # Ignore some warnings
    compiler_options.append '-Wno-return-type'

    success = true
    failure_type = ''

    correct_result = single_smacpp_run(compiler_options, correct)
    catch_bad_result = single_smacpp_run(compiler_options, catch_bad)
    incorrect_result = single_smacpp_run(compiler_options, incorrect)

    if warnings_or_errors?(correct_result) ||
       warnings_or_errors?(catch_bad_result)
      success = false
      failure_type = 'false positive'
    elsif !warnings_or_errors? incorrect_result
      success = false
      failure_type = 'false negative'
    end

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: (correct_result[:time] + catch_bad_result[:time] +
             incorrect_result[:time]),
      raw: {
        correct_result: correct_result,
        catch_bad_result: catch_bad_result,
        incorrect_result: incorrect_result
      }
    }
  elsif tool == :frama
    success = true
    failure_type = ''

    correct_result = single_frama_run(compiler_options, correct)
    catch_bad_result = single_frama_run(compiler_options, catch_bad)
    incorrect_result = single_frama_run(compiler_options, incorrect)

    if warnings_or_errors?(correct_result) ||
       warnings_or_errors?(catch_bad_result)
      success = false
      failure_type = 'false positive'
    elsif !warnings_or_errors? incorrect_result
      success = false
      failure_type = 'false negative'
    end

    results[tool] = {
      success: success,
      failure_type: failure_type,
      time: (correct_result[:time] + catch_bad_result[:time] +
             incorrect_result[:time]),
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

def handle_jm_case(includes, include_folder, file)
  result = {}

  run_test_cases result, includes,
                 File.join(include_folder, 'test_correct', file),
                 File.join(include_folder, 'test_correct_catch_bad', file),
                 File.join(include_folder, 'test_incorrect', file)

  result
end

def jm2018ts_test_case(folder)
  include_folder = File.absolute_path folder

  name_prefix = (folder.split('/').last 2).join '/'

  includes = [include_folder, File.absolute_path('test/data/JM2018TS/common')]

  Dir.entries(folder).each do |f|
    full_path = File.join include_folder, f

    next if File.directory? full_path

    next unless f =~ /\d\d_.*/i

    case_name = name_prefix + '/' + f
    puts "Running test case: #{case_name}"
    result = handle_jm_case includes, include_folder, f

    $results[case_name] = result
  end
end

jm2018ts_test_case 'test/data/JM2018TS/strings/unbounded_copy'
jm2018ts_test_case 'test/data/JM2018TS/strings/overflow'
jm2018ts_test_case 'test/data/JM2018TS/memory/double_free'
jm2018ts_test_case 'test/data/JM2018TS/memory/access_uninit'
jm2018ts_test_case 'test/data/JM2018TS/memory/leak'
jm2018ts_test_case 'test/data/JM2018TS/memory/refer_free'
jm2018ts_test_case 'test/data/JM2018TS/memory/zero_alloc'

puts ''
puts 'Finished running'
File.write 'results.json', JSON.pretty_generate($results)

File.open('results_table.tex', 'w') do |file|
  $results.each do |key, value|
    smacpp = value[:smacpp][:success]
    clang_success = value[:clang][:success]
    frama_success = value[:frama][:success]
    sanitized_key = key.gsub('_', '\_').tr('/', ' ')

    # failure = value[:clang][:failure_type] unless clang_success

    combined_result = clang_success || smacpp

    failure = if combined_result
                ''
              else
                value[:smacpp][:failure_type]
              end

    file.puts "#{sanitized_key} & #{clang_success} & #{smacpp} & " \
              "#{combined_result} & #{frama_success} & #{failure} \\\\"
    file.puts('\hline')
  end
end
