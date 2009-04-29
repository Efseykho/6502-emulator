$:.unshift File.join(File.dirname(__FILE__), ".." )

require "test/unit"
require "tokenizer.rb"



class TestTokenizer < Test::Unit::TestCase
   
  def setup
    # Nothing really
  end
 
  def teardown
    # Nothing really
  end
  
  #do all tests in here for sanity
  def test_init
      assert_raise(ArgumentError ) do 
        tok = Tokenizer.new
      end
      
      tok = Tokenizer.new("test.as")
      assert_equal( 11,tok.buffer.size ) #counts empty lines
      #assert_equal( tok.buffer[6], nil )
      assert_equal( tok.buffer[5], nil )
      assert_equal( tok.buffer[4].to_s, "break:" )
      assert_equal( tok.buffer[3], ["break:", ";"] )
      assert_equal( tok.buffer[2].join(" "), ";This is also a comment" )
      assert_equal( tok.buffer[1].join(" "), ";" )
      assert_equal( tok.buffer[0].join(" "), "; This is a comment" )
  end
    
  def test_get_line
    tok = Tokenizer.new("test.as")
    
    assert( tok.has_more? )
    str = tok.get_line
    assert_equal(str, "; This is a comment")
    
    assert( tok.has_more? )
    str = tok.get_line
    assert_equal(str, ";")
    
    assert( tok.has_more? )
    str = tok.get_line
    assert_equal(str, ";This is also a comment")
    
    assert( tok.has_more? )
    str = tok.get_line
    assert_equal(str, "break: ;")
    
    assert( tok.has_more? )
    str = tok.get_line
    assert_equal(str, "break:")
    
    assert( tok.has_more? )
    str = tok.get_line
    assert( tok.has_more? )
    str = tok.get_line
    assert( tok.has_more? )
    str = tok.get_line
    
    assert( tok.has_more? == false )
    str = tok.get_line
    assert( str == nil )
  end
  
  def test_peek_next
    
    tok = Tokenizer.new("test.as")
    
    
    arr = [";", "This", "is", "a", "comment"]
    arr.each_index { |i|
      #printf("i=#{i}, str[i]=#{arr[i]}\n")
      assert( tok.has_more? )
      str = tok.peek
      assert_equal(str, arr[i])
      str = tok.next
      assert_equal(str, arr[i])
    }
    
    arr = [";"]
    arr.each_index { |i|
      assert( tok.has_more? )
      str = tok.peek
      #printf("str=#{str} temp=#{tok.temp}\n")
      assert_equal(str, arr[i])
      str = tok.next
      assert_equal(str, arr[i])
    }
    
    arr =[";This", "is", "also", "a", "comment"]
    arr.each_index { |i|
      #printf("i=#{i}, str[i]=#{arr[i]}\n")
      assert( tok.has_more? )
      str = tok.peek
      assert_equal(str, arr[i])
      str = tok.next
      assert_equal(str, arr[i])
    }
    
    arr = ["break:", ";"]
    arr.each_index { |i|
      #printf("i=#{i}, str[i]=#{arr[i]}\n")
      assert( tok.has_more? )
      str = tok.peek
      assert_equal(str, arr[i])
      str = tok.next
      assert_equal(str, arr[i])
    }
    
    arr = ["break:"]
    arr.each_index { |i|
      #printf("i=#{i}, str[i]=#{arr[i]}\n")
      assert( tok.has_more? )
      str = tok.peek
      assert_equal(str, arr[i])
      str = tok.next
      assert_equal(str, arr[i])
    }
  end
  
  
  def test_combine_tokens
    tok = Tokenizer.new("test.as")
    tok.combine_tokens
    
    assert( tok.buffer[6] == ["LDA", "$40,LABEL"] )
    assert( tok.buffer[7] == ["LDA", "$40,LABEL"] )
    assert( tok.buffer[8] == ["error,"] )
  end

end #class TestTokenizer < Test::Unit::TestCase