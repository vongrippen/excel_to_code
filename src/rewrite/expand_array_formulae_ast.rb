require_relative '../excel'

class ExpandArrayFormulaeAst
    
  def map(ast)
    return ast unless ast.is_a?(Array)
    operator = ast[0]
    if respond_to?(operator)
      send(operator,*ast[1..-1])
    else
      [operator,*ast[1..-1].map {|a| map(a) }]
    end
  end

  def arithmetic(left,operator,right)
    left = map(left)
    right = map(right)
    return [:arithmetic, left, operator, right] unless array?(left,right)
    
    map_arrays([left,right]) do |arrayed|
      [:arithmetic,arrayed[0],operator,arrayed[1]]
    end
  end

  def comparison(left,operator,right)
    left = map(left)
    right = map(right)
    return [:comparison, left, operator, right] unless array?(left,right)
    
    map_arrays([left,right]) do |arrayed|
      [:comparison,arrayed[0],operator,arrayed[1]]
    end
  end
  
  def string_join(*strings)
    strings = strings.map { |s| map(s) }
    return [:string_join, *strings] unless array?(*strings)
    map_arrays(strings) do |arrayed_strings|
      [:string_join, *arrayed_strings]
    end
  end
  
  def map_arrays(arrays, &block)
    # Turn them into ruby arrays
    arrays = arrays.map { |a| array_ast_to_ruby_array(a) }
    
    # Find the largest one
    max_rows = arrays.max { |a| a.is_a?(Array) ? a.length : 0 }.length
    max_columns = arrays.min { |a| a.is_a?(Array) && a.first.is_a?(Array) ? a.first.length : 0 }.first.length
    
    # Convert any single values into an array of the right size
    arrays = arrays.map { |a| a.is_a?(Array) ? a : Array.new(max_rows, Array.new(max_columns,a)) }
    
    # Convert any single rows into an array of the right size
    arrays = arrays.map { |a| a.length == 1 ? Array.new(max_rows,a.first) : a }
    
    # Convert any single columns into an array of the right size
    arrays = arrays.map { |a| a.first.length == 1 ? Array.new(max_columns,a.flatten(1)).transpose : a }
    
    # Now iterate through
    return [:array, *max_rows.times.map do |row|
      [:row, *max_columns.times.map do |column| 
        block.call(arrays.map do |a|
          a[row][column] || [:error, "#N/A"]
        end)
      end]
    end]
  end
  
  FUNCTIONS_THAT_ACCEPT_RANGES_FOR_ALL_ARGUMENTS = %w{AVERAGE COUNT COUNTA MAX MIN SUM SUMPRODUCT}
  
  def function(name,*arguments)
    if FUNCTIONS_THAT_ACCEPT_RANGES_FOR_ALL_ARGUMENTS.include?(name)
      [:function, name, *arguments.map { |a| map(a) }]
    elsif respond_to?("map_#{name.downcase}")
      send("map_#{name.downcase}",*arguments)
    else
      function_that_does_not_accept_ranges(name,arguments)
    end
  end
  
  def function_that_does_not_accept_ranges(name,arguments)
    arguments = arguments.map { |s| map(s) }
    return [:function, name, *arguments] unless array?(*arguments)
    map_arrays(arguments) do |arrayed_arguments|
      [:function, name, *arrayed_arguments]
    end
  end
    
  private
  
  def array?(*args)
    args.any? { |a| a.first == :array }
  end
  
  def array_ast_to_ruby_array(array_ast)
    return array_ast unless array_ast.first == :array
    array_ast[1..-1].map do |row_ast|
      row_ast[1..-1].map do |cell|
        cell
      end
    end
  end
  
end