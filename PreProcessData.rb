#!/usr/bin/env ruby
puts "Preprocessing test data"

def runSystemCommand(*args)
  system(*args)

  if $?.exitstatus != 0
    puts "Failed to run command: #{args}"
    exit 1
  end
end


def JM2018TSprocess(subdir)

  dir = File.join("test/data/JM2018TS/", subdir)

  if !File.exists? dir
    puts "JM2018TS subdir doesn't exist: #{dir}"
    exit 1
  end

  runSystemCommand("test/data/JM2018TS/preprocess_files.sh", dir)
end

JM2018TSprocess "memory/access_uninit"
JM2018TSprocess "memory/double_free"
JM2018TSprocess "memory/leak"
JM2018TSprocess "memory/refer_free"
JM2018TSprocess "memory/zero_alloc"
JM2018TSprocess "strings/overflow"
JM2018TSprocess "strings/unbounded_copy"

julietFolder = "Juliet_Test_Suite_v1.3_for_C_Cpp"


if !File.exists? "test/data/#{julietFolder}"

  zipName = "#{julietFolder}.zip"

  if !File.exists? "test/data/#{zipName}"
    puts "Juliet v1.3 is missing downloading..."
    
    runSystemCommand "curl",
                     "https://samate.nist.gov/SRD/testsuites/juliet/#{zipName}",
                     "-o", "test/data/#{zipName}"
  end
  
  puts "Unzipping Juliet"

  runSystemCommand "unzip", "test/data/#{zipName}", "-d", "test/data/#{julietFolder}"
end

Dir.chdir("test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C"){
  runSystemCommand "python3", "create_per_cwe_files.py"
}

puts "Finished"
