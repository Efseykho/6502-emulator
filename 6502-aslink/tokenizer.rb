#a simple tokenizer interface
class Tokenizer
  attr_accessor :buffer #buffer of tokens
  attr_accessor :curr_line #current line we're on
  attr_accessor :temp
  
  def initialize( filename )
    @buffer = Array.new
    File.open(filename).each { |line|
      @buffer.push(line)
    }
    
    #split on white space
    @buffer.each_index{ |i|
      if @buffer[i].gsub(" ","").gsub("\n","").size == 0
        @buffer[i] = nil
      else
        @buffer[i] = @buffer[i].split(" ")
      end
    }
    
    #initial step
    @curr_line = -1
    @avail = get_next #is there data available to be read
    @temp = Array.new(@buffer[@curr_line]) if @avail
  end
  
  def has_more?
    return true if @temp != nil  and (@temp.empty? == false)
    return false if @avail == false
    
    ct = @curr_line + 1
    
    while ct < @buffer.size and @buffer[ct] == nil do
      ct += 1
    end
    
    return false if ct == @buffer.size
    return true
  end
  
  def peek
    return nil if has_more? == false
    return nil if @temp == nil
    
    return @temp[0] if (@temp.empty? == false)
    
    #case when array is empty; must load next line into @temp
    if @temp.empty?
      get_next
      @temp = Array.new(@buffer[@curr_line]) if @avail
    end
    
    
    return @temp[0]
  end
  
  #this will break the line numbers, so dont use it unless you have to
  #specifically, dont put back last element in line
  #def put_back(elem)
  #  @temp.unshift(elem)
  #end
  
  #there's a subtelty here
  #mainly, if next is called and an empty @temp results, this must be ok
  #we'll load the new line on next call to 
  def next
    return nil if @temp == nil
    
    if @temp.empty?
      get_next
      @temp = Array.new(@buffer[@curr_line]) if @avail
    end
    
    return @temp.shift if @avail
    
    return nil
  end
  
  #returns rest of current line and attempts to read in next line
  def get_line
    return nil if @temp == nil 
    
    ret = @temp.join(" ")
    get_next
    
    if @avail
      @temp = Array.new(@buffer[@curr_line]) 
    else
      @temp = nil
    end
    
    ret
  end
  
    #PRIVATE
  #this throws out current line and attempts to
  #load in next non-empty lie
  #returns true if there exists more to read (in which case @curr_line set to it)
  #false otherwise
  def get_next
    @curr_line += 1
    
    while @curr_line < @buffer.size and @buffer[@curr_line] == nil do
      @curr_line += 1
    end
    
    if @curr_line == @buffer.size
      @avail = false
      return false
    end
    return true
  end
  
  private :get_next
end #class Tokenizer
