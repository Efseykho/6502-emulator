$:.unshift File.join(File.dirname(__FILE__), ".." )

require "gen_opcode.rb"
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
  
  def test_is_imm_mem
    assert_equal( @assembler.is_imm_mem?("\#\$08"), [Assembler::IMMEDIATE,1,nil,8])
    assert_equal( @assembler.is_imm_mem?("\#\$Fa"), [Assembler::IMMEDIATE,1,nil,250])
    assert_equal( @assembler.is_imm_mem?("\#C27"),[Assembler::IMMEDIATE,1,nil,23])
    assert_equal( @assembler.is_imm_mem?("\#127"),[Assembler::IMMEDIATE,1,nil,127])
    assert_equal( @assembler.is_imm_mem?("\#B101"),[Assembler::IMMEDIATE,1,nil,5])
    
    assert_equal( @assembler.is_imm_mem?("\#C1126"),nil)
    assert_equal( @assembler.is_imm_mem?("\#\$FaH"), nil)
    assert_equal( @assembler.is_imm_mem?("\$Fa"), nil)
  end
  
  def test_is_direct_mem
    assert_equal( @assembler.is_direct_mem?("\$08"), [Assembler::Z_PAGE,1,nil,8])
    assert_equal( @assembler.is_direct_mem?("\$Fa"), [Assembler::Z_PAGE,1,nil,250])
    assert_equal( @assembler.is_direct_mem?("C27"),[Assembler::Z_PAGE,1,nil,23])
    assert_equal( @assembler.is_direct_mem?("127"),[Assembler::Z_PAGE,1,nil,127])
    assert_equal( @assembler.is_direct_mem?("B101"),[Assembler::Z_PAGE,1,nil,5])
    
    assert_equal( @assembler.is_direct_mem?("C1126"), [Assembler::ABSOLUTE,2,nil,598])
    assert_equal( @assembler.is_direct_mem?("\$FaH"), nil)
    
    assert_equal( @assembler.is_direct_mem?("label"), [Assembler::ABSOLUTE,2,nil,"label"])
    assert_equal( @assembler.is_direct_mem?("label_"), [Assembler::ABSOLUTE,2,nil,"label_"])
    assert_equal( @assembler.is_direct_mem?("labe:"),nil)
    assert_equal( @assembler.is_direct_mem?("label15355_32424"),nil) #label >10chars
    assert_equal( @assembler.is_direct_mem?("5label"),nil) #label must start with char
  end
  
  def test_is_pre_indexed_indirect_mem
    assert_equal( @assembler.is_pre_indexed_indirect_mem?("C1126"), nil)
    
    assert_equal( @assembler.is_pre_indexed_indirect_mem?("(C126,X)"), [Assembler::IND_X,1,:X,86])
    assert_equal( @assembler.is_pre_indexed_indirect_mem?("(C1126,X)"), nil) #overflow 1byte
    assert_equal( @assembler.is_pre_indexed_indirect_mem?("(C1126,X"), nil)
    assert_equal( @assembler.is_pre_indexed_indirect_mem?("(C112,Y)"), nil)
    
    assert_equal( @assembler.is_pre_indexed_indirect_mem?("($AF,X)"), [Assembler::IND_X,1,:X,175])
  end
  
  def test_is_post_indexed_indirect_mem
    assert_equal( @assembler.is_post_indexed_indirect_mem?("C1126"), nil)
    
    assert_equal( @assembler.is_post_indexed_indirect_mem?("(C126),Y"), [Assembler::IND_Y,1,:Y,86])
    assert_equal( @assembler.is_post_indexed_indirect_mem?("(C1126),Y"), nil) #overflow 1byte
    assert_equal( @assembler.is_post_indexed_indirect_mem?("(C1126,Y"), nil)
    assert_equal( @assembler.is_post_indexed_indirect_mem?("(C112,X)"), nil)
    
    assert_equal( @assembler.is_post_indexed_indirect_mem?("($AF),Y"), [Assembler::IND_Y,1,:Y,175])
  end  
  
  def test_is_indexed_indirect_mem
    assert_equal( @assembler.is_indexed_indirect_mem?("C1126"), nil)
    
    assert_equal( @assembler.is_indexed_indirect_mem?("C1126,X"), [Assembler::ABS_X,2,:X,598]  )
    assert_equal( @assembler.is_indexed_indirect_mem?("C1126,Y"), [Assembler::ABS_Y,2,:Y,598]  )
    assert_equal( @assembler.is_indexed_indirect_mem?("C1126,Z"), nil  )
    
    assert_equal( @assembler.is_indexed_indirect_mem?("$FF,X"), [Assembler::Z_PAGE_X,1,:X,255]   )
    
    assert_equal( @assembler.is_indexed_indirect_mem?("HOME,X"), [Assembler::ABS_X,2,:X,"HOME"]   )
    assert_equal( @assembler.is_indexed_indirect_mem?("HM,Y"), [Assembler::ABS_Y,2,:Y,"HM"]   )
    
    assert_equal( @assembler.is_indexed_indirect_mem?("HM,Z"), nil   )
    assert_equal( @assembler.is_indexed_indirect_mem?("($FF),X"), nil )
    assert_equal( @assembler.is_indexed_indirect_mem?("($FF,X)"), nil )
  end
  
  def test_is_indirect_mem
    assert_equal( @assembler.is_indirect_mem?("C1126"), nil)
    
    assert_equal( @assembler.is_indirect_mem?("(C1126)"), [Assembler::INDIRECT,2,nil,598] )
    assert_equal( @assembler.is_indirect_mem?("($FA)"), [Assembler::INDIRECT,2,nil,250] )
    assert_equal( @assembler.is_indirect_mem?("($00FA)"), [Assembler::INDIRECT,2,nil,250] )
    
    assert_equal( @assembler.is_indirect_mem?("(LBL)"), [Assembler::INDIRECT,2,nil,"LBL"] )
    assert_equal( @assembler.is_indirect_mem?("(LBL:)"), nil )
  end
  
end
  