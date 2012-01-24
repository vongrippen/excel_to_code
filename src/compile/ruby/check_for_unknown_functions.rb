require_relative 'map_formulae_to_ruby'

class CheckForUnknownFunctions
  
  attr_accessor :settable
  
  def self.rewrite(*args)
    self.new.rewrite(*args)
  end
  
  def check(input,output)
    self.settable ||= lambda { |ref| false }
    input.lines do |line|
      line.scan(/\[:function, "(.*?)"/).each do |match|
        output.puts $1 unless MapFormulaeToRuby::FUNCTIONS.has_key?($1)
      end
    end
  end
  
end
