$:.unshift File.join(File.dirname(__FILE__), ".." )

require "test/unit"
require "6502-assembler.rb"

class TestAssembler < Test::Unit::TestCase
   
  def setup
    @assembler = Assembler.new
  end
 
  def teardown
    # Nothing really
  end
  
  #test logic for parsing comments
  def test_comment
    assert_equal( @assembler.is_comment?(""), nil )
    assert_equal( @assembler.is_comment?("token"), nil )
    assert_equal( @assembler.is_comment?(";"), ";" )
    assert_equal( @assembler.is_comment?(";This"), ";This" )
    assert_equal( @assembler.is_comment?("token;"), nil )
  end
  
  def test_label
    assert_equal( @assembler.is_label?(""), nil )
    assert_equal( @assembler.is_label?("label"), nil )
    assert_equal( @assembler.is_label?(";"), nil )
    assert_equal( @assembler.is_label?("label:"), "label" )
    assert_equal( @assembler.is_label?("label:o"), nil )
  end
  
  def test_get_number_system
    assert_equal( @assembler.get_number_system("$0A"), 10 )
    assert_equal( @assembler.get_number_system("$FF"), 255 )
    assert_equal( @assembler.get_number_system("$0"), 0 )
    assert_equal( @assembler.get_number_system("$9"), 9 )
    
    assert_equal( @assembler.get_number_system("C07"), 7 )
    assert_equal( @assembler.get_number_system("C34"), 28 )
    assert_equal( @assembler.get_number_system("C0"), 0 )
    assert_equal( @assembler.get_number_system("C5"), 5 )
    
    assert_equal( @assembler.get_number_system("B01"), 1 )
    assert_equal( @assembler.get_number_system("B101"), 5 )
    assert_equal( @assembler.get_number_system("B0"), 0 )
    assert_equal( @assembler.get_number_system("B111"), 7 )
    
    assert_equal( @assembler.get_number_system("01"), 1 )
    assert_equal( @assembler.get_number_system("101"), 101)
    assert_equal( @assembler.get_number_system("0"), 0 )
    assert_equal( @assembler.get_number_system("111"), 111 )
  end
  
  def test_is_number
    assert_equal( @assembler.is_number?("$5"), 5 )
    assert_equal( @assembler.is_number?("$05"), 5 )
    assert_equal( @assembler.is_number?("$50"), 80 )
    assert_equal( @assembler.is_number?("$0A"), 10)
    assert_equal( @assembler.is_number?("$FF"), 255)
    
    assert_equal( @assembler.is_number?("$5H"), nil)
    assert_equal( @assembler.is_number?("$AH"), nil )
    assert_equal( @assembler.is_number?("$50Z"), nil )
    assert_equal( @assembler.is_number?("$Z12"), nil)
    assert_equal( @assembler.is_number?("$A^"), nil)
    
    assert_equal( @assembler.is_number?("C5"), 5 )
    assert_equal( @assembler.is_number?("C05"), 5 )
    assert_equal( @assembler.is_number?("C50"), 40 )
    assert_equal( @assembler.is_number?("C07"), 7)
    assert_equal( @assembler.is_number?("C"), 0)
    
    assert_equal( @assembler.is_number?("CD"), nil )
    assert_equal( @assembler.is_number?("C8"), nil )
    assert_equal( @assembler.is_number?("CC6"), nil )
    assert_equal( @assembler.is_number?("C07^"), nil )
    
    assert_equal( @assembler.is_number?("B2"), nil )
    assert_equal( @assembler.is_number?("BA"), nil )
    assert_equal( @assembler.is_number?("B1^"), nil )
    assert_equal( @assembler.is_number?("B10R"), nil )
    
    assert_equal( @assembler.is_number?("05"), 5 )
    assert_equal( @assembler.is_number?("50"), 50 )
    assert_equal( @assembler.is_number?("07"), 7)
    assert_equal( @assembler.is_number?("77"), 77)
    
    assert_equal( @assembler.is_number?("0H"), nil)
    assert_equal( @assembler.is_number?("0A"), nil)
  end
  
  def test_is_8bit_number
    assert_equal( @assembler.is_8bit_number?("$FF"), 255)
    assert_equal( @assembler.is_8bit_number?("$0A"), 10)
    assert_equal( @assembler.is_8bit_number?("$F0"), 240)
    
    assert_equal( @assembler.is_8bit_number?("C1126"), nil)
    assert_equal( @assembler.is_8bit_number?("C07"), 7)
    
    assert_equal( @assembler.is_8bit_number?("$FG"), nil)
    
    assert_equal( @assembler.is_8bit_number?("255"), 255)
    assert_equal( @assembler.is_8bit_number?("257"), nil)
  end

  def test_is_16bit_number
    assert_equal( @assembler.is_16bit_number?("C1126"), 598)
    assert_equal( @assembler.is_16bit_number?("255"), 255)
    assert_equal( @assembler.is_16bit_number?("257"), 257)
    
    assert_equal( @assembler.is_16bit_number?("$FG"), nil)
  end
  
end
  