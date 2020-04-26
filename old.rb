#!/usr/bin/ruby --disable-gems

require "ostruct"
require "optparse"

DEFAULT_ARGS = ["/on", "/b"]

def windir(args, opts)
  rt = []
  reverse = opts.reverse
  mustcache = (reverse == true)
  precmd = []
  if opts.shortnames then
    precmd.push("/x")
  end
  sh = ["cmd", "/c", "dir", *precmd, *DEFAULT_ARGS, *args].map(&:to_s)
  IO.popen(sh) do |fh|
    fh.each_line do |line|
      item = line.strip
      if mustcache then
        rt.push(item)
      else
        yield item
      end
    end
  end
  if reverse then
    rt.reverse!
  end
  rt.each do |item|
    yield item
  end
end

begin
  opts = OpenStruct.new({
    reverse: false,
    shortnames: false,
  })
  ARGV.options do |prs|
    prs.on("-r", "--reverse"){
      opts.reverse = true
    }
    prs.on("-s", "--short"){
      opts.shortnames = true
    }
    prs.parse!
  end
  windir(ARGV, opts) do |item|
    $stdout.puts(item)
  end
end


