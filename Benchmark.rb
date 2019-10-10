#!/usr/bin/env ruby
# Runs all the tests for seeing how smacpp compares to clang analyzer and also if
# they were both used how good the results were
require 'open3'
require 'json'
require 'fileutils'

$results = {}

FileUtils.mkdir_p 'temp'

def run_process(*args)
  Dir.chdir 'temp' do
    stdout, stderr, status = Open3.capture3(*args)
    [stdout, stderr, status.exitstatus]
  end
end

def single_clang_run(compiler_options, file)
  stdout, stderr, status = run_process 'clang', '--analyze', *compiler_options, file
  # stdout, stderr, status = run_process "scan-build", "clang", *compiler_options, correct
  # stdout, stderr, status = run_process "clang", *compiler_options, correct, "-cc1", "-analyze",
  #                                     "-analyzer-checker=*",

  error_count = 0
  warning_count = 0
  
  if (match = stderr.match(/(\d+)\s+error\sgenerated/i))
    error_count = match.captures[0].to_i
  end
  if (match = stderr.match(/(\d+)\s+warning\sgenerated/i))
    warning_count = match.captures[0].to_i
  end
  
  {
    out: stdout,
    stderr: stderr,
    exit_code: status,
    error_count: error_count,
    warning_count: warning_count
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

  run_test_cases_for_single_tool results, :clang, includes.map{|i| ["-I", i]}.flatten,
                                 correct, catch_bad, incorrect
  # run_test_cases_for_single_tool :smacpp, correct, catch_bad, incorrect
end

def jm2018ts_test_case(folder)
  includeFolder = File.absolute_path folder

  namePrefix = (folder.split("/").last 2).join "/"

  includes = [includeFolder, File.absolute_path("test/data/JM2018TS/common")]

  Dir.entries(folder).each{|f|

    fullPath = File.join includeFolder, f

    if File.directory? fullPath
      next
    end

    if f =~ /\d\d_.*/i
      caseName = namePrefix + "/" + f
      puts "Running test case: #{caseName}"
      result = {}

      run_test_cases result, includes, File.join(includeFolder, "test_correct", f),
                   File.join(includeFolder, "test_correct_catch_bad", f),
                   File.join(includeFolder, "test_incorrect", f)

      $results[caseName] = result
    end
  }
  
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
puts JSON.pretty_generate $results
