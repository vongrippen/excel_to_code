require_relative '../spec_helper'

describe ExtractSimpleFormulae do
  
  it "should create a flat file with one string per formula, in the format: reference\tformula; it should skip blank formulae" do
    input = excel_fragment 'FormulaeTypes.xml'
    output = StringIO.new
    ExtractSimpleFormulae.extract(input,output)
    expected_output = <<END
B1\t1+1
B2\tCOSH(2*PI())
END
    output.string.should == expected_output
  end
end
