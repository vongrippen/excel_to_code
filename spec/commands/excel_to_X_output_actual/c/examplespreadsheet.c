// /Users/tamc/Documents/github/excel_to_code/spec/test_data/ExampleSpreadsheet.xlsx approximately translated into C
// First we have c versions of all the excel functions that we know
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

// FIXME: Extract a header file

// I predefine an array of ExcelValues to store calculations
// Probably bad practice. At the very least, I should make it
// link to the cell reference in some way.
#define MAX_EXCEL_VALUE_HEAP_SIZE 1000000
#define MAX_MEMORY_TO_BE_FREED_HEAP_SIZE 1000000

#define true 1
#define false 0

// These are the various types of excel cell, plus ExcelRange which allows the passing of arrays of cells
typedef enum {ExcelEmpty, ExcelNumber, ExcelString, ExcelBoolean, ExcelError, ExcelRange} ExcelType;

struct excel_value {
	ExcelType type;
	
	double number; // Used for numbers and for error types
	char *string; // Used for strings
	
	// The following three are used for ranges
	void *array;
	int rows;
	int columns;
};

typedef struct excel_value ExcelValue;


// These are used in the SUMIF and SUMIFS criteria (e.g., when passed a string like "<20")
typedef enum {LessThan, LessThanOrEqual, Equal, NotEqual, MoreThanOrEqual, MoreThan} ExcelComparisonType;

struct excel_comparison {
	ExcelComparisonType type;
	ExcelValue comparator;
};

typedef struct excel_comparison ExcelComparison;

// Headers
static ExcelValue more_than(ExcelValue a_v, ExcelValue b_v);
static ExcelValue more_than_or_equal(ExcelValue a_v, ExcelValue b_v);
static ExcelValue not_equal(ExcelValue a_v, ExcelValue b_v);
static ExcelValue less_than(ExcelValue a_v, ExcelValue b_v);
static ExcelValue less_than_or_equal(ExcelValue a_v, ExcelValue b_v);
static ExcelValue find_2(ExcelValue string_to_look_for_v, ExcelValue string_to_look_in_v);
static ExcelValue find(ExcelValue string_to_look_for_v, ExcelValue string_to_look_in_v, ExcelValue position_to_start_at_v);
static ExcelValue iferror(ExcelValue value, ExcelValue value_if_error);
static ExcelValue excel_index(ExcelValue array_v, ExcelValue row_number_v, ExcelValue column_number_v);
static ExcelValue excel_index_2(ExcelValue array_v, ExcelValue row_number_v);
static ExcelValue left(ExcelValue string_v, ExcelValue number_of_characters_v);
static ExcelValue left_1(ExcelValue string_v);
static ExcelValue max(int number_of_arguments, ExcelValue *arguments);
static ExcelValue min(int number_of_arguments, ExcelValue *arguments);
static ExcelValue mod(ExcelValue a_v, ExcelValue b_v);
static ExcelValue negative(ExcelValue a_v);
static ExcelValue pmt(ExcelValue rate_v, ExcelValue number_of_periods_v, ExcelValue present_value_v);
static ExcelValue power(ExcelValue a_v, ExcelValue b_v);
static ExcelValue excel_round(ExcelValue number_v, ExcelValue decimal_places_v);
static ExcelValue rounddown(ExcelValue number_v, ExcelValue decimal_places_v);
static ExcelValue roundup(ExcelValue number_v, ExcelValue decimal_places_v);
static ExcelValue string_join(int number_of_arguments, ExcelValue *arguments);
static ExcelValue subtotal(ExcelValue type, int number_of_arguments, ExcelValue *arguments);
static ExcelValue sumifs(ExcelValue sum_range_v, int number_of_arguments, ExcelValue *arguments);
static ExcelValue sumif(ExcelValue check_range_v, ExcelValue criteria_v, ExcelValue sum_range_v );
static ExcelValue sumif_2(ExcelValue check_range_v, ExcelValue criteria_v);
static ExcelValue sumproduct(int number_of_arguments, ExcelValue *arguments);
static ExcelValue vlookup_3(ExcelValue lookup_value_v,ExcelValue lookup_table_v, ExcelValue column_number_v);
static ExcelValue vlookup(ExcelValue lookup_value_v,ExcelValue lookup_table_v, ExcelValue column_number_v, ExcelValue match_type_v);

// My little heap for excel values
ExcelValue cells[MAX_EXCEL_VALUE_HEAP_SIZE];
int cell_counter = 0;

#define HEAPCHECK if(cell_counter >= MAX_EXCEL_VALUE_HEAP_SIZE) { printf("ExcelValue heap full. Edit MAX_EXCEL_VALUE_HEAP_SIZE in the c source code."); exit(-1); }

// My little heap for keeping pointers to memory that I need to reclaim
void *memory_that_needs_to_be_freed[MAX_MEMORY_TO_BE_FREED_HEAP_SIZE];
int memory_that_needs_to_be_freed_counter = 0;

#define MEMORY_THAT_NEEDS_TO_BE_FREED_HEAP_CHECK 

static void free_later(void *pointer) {
	memory_that_needs_to_be_freed[memory_that_needs_to_be_freed_counter] = pointer;
	memory_that_needs_to_be_freed_counter++;
	if(memory_that_needs_to_be_freed_counter >= MAX_MEMORY_TO_BE_FREED_HEAP_SIZE) { 
		printf("Memory that needs to be freed heap full. Edit MAX_MEMORY_TO_BE_FREED_HEAP_SIZE in the c source code"); 
		exit(-1);
	}
}

static void free_all_allocated_memory() {
	int i;
	for(i = 0; i < memory_that_needs_to_be_freed_counter; i++) {
		free(memory_that_needs_to_be_freed[i]);
	}
	memory_that_needs_to_be_freed_counter = 0;
}

// The object initializers
static ExcelValue new_excel_number(double number) {
	cell_counter++;
	HEAPCHECK
	ExcelValue new_cell = 	cells[cell_counter];
	new_cell.type = ExcelNumber;
	new_cell.number = number;
	return new_cell;
};

static ExcelValue new_excel_string(char *string) {
	cell_counter++;
	HEAPCHECK
	ExcelValue new_cell = 	cells[cell_counter];
	new_cell.type = ExcelString;
	new_cell.string = string;
	return new_cell;
};

static ExcelValue new_excel_range(void *array, int rows, int columns) {
	cell_counter++;
	HEAPCHECK
	ExcelValue new_cell = cells[cell_counter];
	new_cell.type = ExcelRange;
	new_cell.array = array;
	new_cell.rows = rows;
	new_cell.columns = columns;
	return new_cell;
};

static void * new_excel_value_array(int size) {
	ExcelValue *pointer = malloc(sizeof(ExcelValue)*size); // Freed later
	if(pointer == 0) {
		printf("Out of memory\n");
		exit(-1);
	}
	free_later(pointer);
	return pointer;
};

// Constants
const ExcelValue BLANK = {.type = ExcelEmpty, .number = 0};

const ExcelValue ZERO = {.type = ExcelNumber, .number = 0};
const ExcelValue ONE = {.type = ExcelNumber, .number = 1};
const ExcelValue TWO = {.type = ExcelNumber, .number = 2};
const ExcelValue THREE = {.type = ExcelNumber, .number = 3};
const ExcelValue FOUR = {.type = ExcelNumber, .number = 4};
const ExcelValue FIVE = {.type = ExcelNumber, .number = 5};
const ExcelValue SIX = {.type = ExcelNumber, .number = 6};
const ExcelValue SEVEN = {.type = ExcelNumber, .number = 7};
const ExcelValue EIGHT = {.type = ExcelNumber, .number = 8};
const ExcelValue NINE = {.type = ExcelNumber, .number = 9};
const ExcelValue TEN = {.type = ExcelNumber, .number = 10};

// Booleans
const ExcelValue TRUE = {.type = ExcelBoolean, .number = true };
const ExcelValue FALSE = {.type = ExcelBoolean, .number = false };

// Errors
const ExcelValue VALUE = {.type = ExcelError, .number = 0};
const ExcelValue NAME = {.type = ExcelError, .number = 1};
const ExcelValue DIV0 = {.type = ExcelError, .number = 2};
const ExcelValue REF = {.type = ExcelError, .number = 3};
const ExcelValue NA = {.type = ExcelError, .number = 4};

// This is the error flag
static int conversion_error = 0;

// Helpful for debugging
static void inspect_excel_value(ExcelValue v) {
	ExcelValue *array;
	int i, j, k;
	switch (v.type) {
  	  case ExcelNumber:
		  printf("Number: %f\n",v.number);
		  break;
	  case ExcelBoolean:
		  if(v.number == true) {
			  printf("True\n");
		  } else if(v.number == false) {
			  printf("False\n");
		  } else {
			  printf("Boolean with undefined state %f\n",v.number);
		  }
		  break;
	  case ExcelEmpty:
	  	if(v.number == 0) {
	  		printf("Empty\n");
		} else {
			printf("Empty with unexpected state %f\n",v.number);	
		}
		break;
	  case ExcelRange:
		 printf("Range rows: %d, columns: %d\n",v.rows,v.columns);
		 array = v.array;
		 for(i = 0; i < v.rows; i++) {
			 printf("Row %d:\n",i+1);
			 for(j = 0; j < v.columns; j++ ) {
				 printf("%d ",j+1);
				 k = (i * v.columns) + j;
				 inspect_excel_value(array[k]);
			 }
		 }
		 break;
	  case ExcelString:
		 printf("String: '%s'\n",v.string);
		 break;
	  case ExcelError:
		 printf("Error number %f ",v.number);
		 switch( (int)v.number) {
			 case 0: printf("VALUE\n"); break;
			 case 1: printf("NAME\n"); break;
			 case 2: printf("DIV0\n"); break;
			 case 3: printf("REF\n"); break;
			 case 4: printf("NA\n"); break;
		 }
		 break;
    default:
      printf("Type %d not recognised",v.type);
	 };
}

// Extracts numbers from ExcelValues
// Excel treats empty cells as zero
static double number_from(ExcelValue v) {
	char *s;
	char *p;
	double n;
	ExcelValue *array;
	switch (v.type) {
  	  case ExcelNumber:
	  case ExcelBoolean: 
	  	return v.number;
	  case ExcelEmpty: 
	  	return 0;
	  case ExcelRange: 
		 array = v.array;
	     return number_from(array[0]);
	  case ExcelString:
 	 	s = v.string;
		if (s == NULL || *s == '\0' || isspace(*s)) {
			return 0;
		}	        
		n = strtod (s, &p);
		if(*p == '\0') {
			return n;
		}
		conversion_error = 1;
		return 0;
	  case ExcelError:
	  	return 0;
  }
  return 0;
}

#define NUMBER(value_name, name) double name; if(value_name.type == ExcelError) { return value_name; }; name = number_from(value_name);
#define CHECK_FOR_CONVERSION_ERROR 	if(conversion_error) { conversion_error = 0; return VALUE; };
#define CHECK_FOR_PASSED_ERROR(name) 	if(name.type == ExcelError) return name;
	
static ExcelValue excel_abs(ExcelValue a_v) {
	CHECK_FOR_PASSED_ERROR(a_v)	
	NUMBER(a_v, a)
	CHECK_FOR_CONVERSION_ERROR
	
	if(a >= 0.0 ) {
		return a_v;
	} else {
		return new_excel_number(-a);
	}
}

static ExcelValue add(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)
	NUMBER(a_v, a)
	NUMBER(b_v, b)
	CHECK_FOR_CONVERSION_ERROR
	return new_excel_number(a + b);
}

static ExcelValue excel_and(int array_size, ExcelValue *array) {
	int i;
	ExcelValue current_excel_value, array_result;
	
	for(i=0;i<array_size;i++) {
		current_excel_value = array[i];
		switch (current_excel_value.type) {
	  	  case ExcelNumber: 
		  case ExcelBoolean: 
			  if(current_excel_value.number == false) return FALSE;
			  break;
		  case ExcelRange: 
		  	array_result = excel_and( current_excel_value.rows * current_excel_value.columns, current_excel_value.array );
			if(array_result.type == ExcelError) return array_result;
			if(array_result.type == ExcelBoolean && array_result.number == false) return FALSE;
			break;
		  case ExcelString:
		  case ExcelEmpty:
			 break;
		  case ExcelError:
			 return current_excel_value;
			 break;
		 }
	 }
	 return TRUE;
}

struct average_result {
	double sum;
	double count;
	int has_error;
	ExcelValue error;
};
	
static struct average_result calculate_average(int array_size, ExcelValue *array) {
	double sum = 0;
	double count = 0;
	int i;
	ExcelValue current_excel_value;
	struct average_result array_result, r;
		 
	for(i=0;i<array_size;i++) {
		current_excel_value = array[i];
		switch (current_excel_value.type) {
	  	  case ExcelNumber:
			  sum += current_excel_value.number;
			  count++;
			  break;
		  case ExcelRange: 
		  	array_result = calculate_average( current_excel_value.rows * current_excel_value.columns, current_excel_value.array );
			if(array_result.has_error == true) return array_result;
			sum += array_result.sum;
			count += array_result.count;
			break;
		  case ExcelBoolean: 
		  case ExcelString:
		  case ExcelEmpty:
			 break;
		  case ExcelError:
			 r.has_error = true;
			 r.error = current_excel_value;
			 return r;
			 break;
		 }
	}
	r.count = count;
	r.sum = sum;
	r.has_error = false;
	return r;
}

static ExcelValue average(int array_size, ExcelValue *array) {
	struct average_result r = calculate_average(array_size, array);
	if(r.has_error == true) return r.error;
	if(r.count == 0) return DIV0;
	return new_excel_number(r.sum/r.count);
}

static ExcelValue choose(ExcelValue index_v, int array_size, ExcelValue *array) {
	CHECK_FOR_PASSED_ERROR(index_v)

	int index = (int) number_from(index_v);
	CHECK_FOR_CONVERSION_ERROR	
	int i;
	for(i=0;i<array_size;i++) {
		if(array[i].type == ExcelError) return array[i];
	}
	if(index < 1) return VALUE;
	if(index > array_size) return VALUE;
	return array[index-1];
}	

static ExcelValue count(int array_size, ExcelValue *array) {
	int i;
	int n = 0;
	ExcelValue current_excel_value;
	
	for(i=0;i<array_size;i++) {
		current_excel_value = array[i];
		switch (current_excel_value.type) {
	  	  case ExcelNumber:
		  	n++;
			break;
		  case ExcelRange: 
		  	n += count( current_excel_value.rows * current_excel_value.columns, current_excel_value.array ).number;
			break;
  		  case ExcelBoolean: 			
		  case ExcelString:
		  case ExcelEmpty:
		  case ExcelError:
			 break;
		 }
	 }
	 return new_excel_number(n);
}

static ExcelValue counta(int array_size, ExcelValue *array) {
	int i;
	int n = 0;
	ExcelValue current_excel_value;
	
	for(i=0;i<array_size;i++) {
		current_excel_value = array[i];
    switch(current_excel_value.type) {
  	  case ExcelNumber:
      case ExcelBoolean:
      case ExcelString:
  	  case ExcelError:
        n++;
        break;
      case ExcelRange: 
	  	  n += counta( current_excel_value.rows * current_excel_value.columns, current_excel_value.array ).number;
        break;
  	  case ExcelEmpty:
  		  break;
    }
	 }
	 return new_excel_number(n);
}

static ExcelValue divide(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)
	NUMBER(a_v, a)
	NUMBER(b_v, b)
	CHECK_FOR_CONVERSION_ERROR
	if(b == 0) return DIV0;
	return new_excel_number(a / b);
}

static ExcelValue excel_equal(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)

	if(a_v.type != b_v.type) return FALSE;
	
	switch (a_v.type) {
  	  case ExcelNumber:
	  case ExcelBoolean: 
	  case ExcelEmpty: 
			if(a_v.number != b_v.number) return FALSE;
			return TRUE;
	  case ExcelString:
	  	if(strcasecmp(a_v.string,b_v.string) != 0 ) return FALSE;
		return TRUE;
  	  case ExcelError:
		return a_v;
  	  case ExcelRange:
  		return NA;
  }
  return FALSE;
}

static ExcelValue not_equal(ExcelValue a_v, ExcelValue b_v) {
	ExcelValue result = excel_equal(a_v, b_v);
	if(result.type == ExcelBoolean) {
		if(result.number == 0) return TRUE;
		return FALSE;
	}
	return result;
}

static ExcelValue excel_if(ExcelValue condition, ExcelValue true_case, ExcelValue false_case ) {
	CHECK_FOR_PASSED_ERROR(condition)
	CHECK_FOR_PASSED_ERROR(true_case)
	CHECK_FOR_PASSED_ERROR(false_case)
	
	switch (condition.type) {
  	  case ExcelBoolean:
  	  	if(condition.number == true) return true_case;
  	  	return false_case;
  	  case ExcelNumber:
		if(condition.number == false) return false_case;
		return true_case;
	  case ExcelEmpty: 
		return false_case;
	  case ExcelString:
	  	return VALUE;
  	  case ExcelError:
		return condition;
  	  case ExcelRange:
  		return VALUE;
  }
  return condition;
}

static ExcelValue excel_if_2(ExcelValue condition, ExcelValue true_case ) {
	return excel_if( condition, true_case, FALSE );
}

static ExcelValue excel_index(ExcelValue array_v, ExcelValue row_number_v, ExcelValue column_number_v) {
	CHECK_FOR_PASSED_ERROR(array_v)
	CHECK_FOR_PASSED_ERROR(row_number_v)
	CHECK_FOR_PASSED_ERROR(column_number_v)
		
	ExcelValue *array;
	int rows;
	int columns;
	
	NUMBER(row_number_v, row_number)
	NUMBER(column_number_v, column_number)
	CHECK_FOR_CONVERSION_ERROR
	
	if(array_v.type == ExcelRange) {
		array = array_v.array;
		rows = array_v.rows;
		columns = array_v.columns;
	} else {
		ExcelValue tmp_array[] = {array_v};
		array = tmp_array;
		rows = 1;
		columns = 1;
	}
	
	if(row_number > rows) return REF;
	if(column_number > columns) return REF;
		
	if(row_number == 0) { // We need the whole column
		if(column_number < 1) return REF;
		ExcelValue *result = (ExcelValue *) new_excel_value_array(rows);
		int result_index = 0;
		ExcelValue r;
		int array_index;
		int i;
		for(i = 0; i < rows; i++) {
			array_index = (i*columns) + column_number - 1;
			r = array[array_index];
			if(r.type == ExcelEmpty) {
				result[result_index] = ZERO;
			} else {
				result[result_index] = r;
			}			
			result_index++;
		}
		return new_excel_range(result,rows,1);
	} else if(column_number == 0 ) { // We need the whole row
		if(row_number < 1) return REF;
		ExcelValue *result = (ExcelValue*) new_excel_value_array(columns);
		ExcelValue r;
		int row_start = ((row_number-1)*columns);
		int row_finish = row_start + columns;
		int result_index = 0;
		int i;
		for(i = row_start; i < row_finish; i++) {
			r = array[i];
			if(r.type == ExcelEmpty) {
				result[result_index] = ZERO;
			} else {
				result[result_index] = r;
			}
			result_index++;
		}
		return new_excel_range(result,1,columns);
	} else { // We need a precise point
		if(row_number < 1) return REF;
		if(column_number < 1) return REF;
		int position = ((row_number - 1) * columns) + column_number - 1;
		ExcelValue result = array[position];
		if(result.type == ExcelEmpty) return ZERO;
		return result;
	}
	
	return FALSE;
};

static ExcelValue excel_index_2(ExcelValue array_v, ExcelValue offset) {
	if(array_v.type == ExcelRange) {
		if(array_v.rows == 1) {
			return excel_index(array_v,ONE,offset);
		} else if (array_v.columns == 1) {
			return excel_index(array_v,offset,ONE);
		} else {
			return REF;
		}
	} else if (offset.type == ExcelNumber && offset.number == 1) {
		return array_v;
	} else {
		return REF;
	}
	return REF;
};


static ExcelValue excel_match(ExcelValue lookup_value, ExcelValue lookup_array, ExcelValue match_type ) {
	CHECK_FOR_PASSED_ERROR(lookup_value)
	CHECK_FOR_PASSED_ERROR(lookup_array)
	CHECK_FOR_PASSED_ERROR(match_type)
		
	// Blanks are treaked as zeros
	if(lookup_value.type == ExcelEmpty) lookup_value = ZERO;

	// Setup the array
	ExcelValue *array;
	int size;
	if(lookup_array.type == ExcelRange) {
		// Check that the range is a row or column rather than an area
		if((lookup_array.rows == 1) || (lookup_array.columns == 1)) {
			array = lookup_array.array;
			size = lookup_array.rows * lookup_array.columns;
		} else {
			// return NA error if covers an area.
			return NA;
		};
	} else {
		// Need to wrap the argument up as an array
		size = 1;
		ExcelValue tmp_array[1] = {lookup_array};
		array = tmp_array;
	}
    
	int type = (int) number_from(match_type);
	CHECK_FOR_CONVERSION_ERROR;
	
	int i;
	ExcelValue x;
	
	switch(type) {
		case 0:
			for(i = 0; i < size; i++ ) {
				x = array[i];
				if(x.type == ExcelEmpty) x = ZERO;
				if(excel_equal(lookup_value,x).number == true) return new_excel_number(i+1);
			}
			return NA;
			break;
		case 1:
			for(i = 0; i < size; i++ ) {
				x = array[i];
				if(x.type == ExcelEmpty) x = ZERO;
				if(more_than(x,lookup_value).number == true) {
					if(i==0) return NA;
					return new_excel_number(i);
				}
			}
			return new_excel_number(size);
			break;
		case -1:
			for(i = 0; i < size; i++ ) {
				x = array[i];
				if(x.type == ExcelEmpty) x = ZERO;
				if(less_than(x,lookup_value).number == true) {
					if(i==0) return NA;
					return new_excel_number(i);
				}
			}
			return new_excel_number(size-1);
			break;
	}
	return NA;
}

static ExcelValue excel_match_2(ExcelValue lookup_value, ExcelValue lookup_array ) {
	return excel_match(lookup_value,lookup_array,new_excel_number(0));
}

static ExcelValue find(ExcelValue find_text_v, ExcelValue within_text_v, ExcelValue start_number_v) {
	CHECK_FOR_PASSED_ERROR(find_text_v)
	CHECK_FOR_PASSED_ERROR(within_text_v)
	CHECK_FOR_PASSED_ERROR(start_number_v)

	char *find_text;	
	char *within_text;
	char *within_text_offset;
	char *result;
	int start_number = number_from(start_number_v);
	CHECK_FOR_CONVERSION_ERROR

	// Deal with blanks 
	if(within_text_v.type == ExcelString) {
		within_text = within_text_v.string;
	} else if( within_text_v.type == ExcelEmpty) {
		within_text = "";
	}

	if(find_text_v.type == ExcelString) {
		find_text = find_text_v.string;
	} else if( find_text_v.type == ExcelEmpty) {
		return start_number_v;
	}
	
	// Check length
	if(start_number < 1) return VALUE;
	if(start_number > strlen(within_text)) return VALUE;
	
	// Offset our within_text pointer
	// FIXME: No way this is utf-8 compatible
	within_text_offset = within_text + (start_number - 1);
	result = strstr(within_text_offset,find_text);
	if(result) {
		return new_excel_number(result - within_text + 1);
	}
	return VALUE;
}

static ExcelValue find_2(ExcelValue string_to_look_for_v, ExcelValue string_to_look_in_v) {
	return find(string_to_look_for_v, string_to_look_in_v, ONE);
};

static ExcelValue left(ExcelValue string_v, ExcelValue number_of_characters_v) {
	CHECK_FOR_PASSED_ERROR(string_v)
	CHECK_FOR_PASSED_ERROR(number_of_characters_v)
	if(string_v.type == ExcelEmpty) return BLANK;
	if(number_of_characters_v.type == ExcelEmpty) return BLANK;
	
	int number_of_characters = (int) number_from(number_of_characters_v);
	CHECK_FOR_CONVERSION_ERROR

	char *string;
	int string_must_be_freed = 0;
	switch (string_v.type) {
  	  case ExcelString:
  		string = string_v.string;
  		break;
  	  case ExcelNumber:
		  string = malloc(20); // Freed
		  if(string == 0) {
			  printf("Out of memory");
			  exit(-1);
		  }
		  string_must_be_freed = 1;
		  snprintf(string,20,"%f",string_v.number);
		  break;
	  case ExcelBoolean:
	  	if(string_v.number == true) {
	  		string = "TRUE";
		} else {
			string = "FALSE";
		}
		break;
	  case ExcelEmpty:	  	 
  	  case ExcelError:
  	  case ExcelRange:
		return string_v;
	}
	
	char *left_string = malloc(number_of_characters+1); // Freed
	if(left_string == 0) {
	  printf("Out of memory");
	  exit(-1);
	}
	free_later(left_string);
	memcpy(left_string,string,number_of_characters);
	left_string[number_of_characters] = '\0';
	if(string_must_be_freed == 1) {
		free(string);
	}
	return new_excel_string(left_string);
}

static ExcelValue left_1(ExcelValue string_v) {
	return left(string_v, ONE);
}

static ExcelValue iferror(ExcelValue value, ExcelValue value_if_error) {
	if(value.type == ExcelError) return value_if_error;
	return value;
}

static ExcelValue more_than(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)

	switch (a_v.type) {
  	  case ExcelNumber:
	  case ExcelBoolean: 
	  case ExcelEmpty:
		if((b_v.type == ExcelNumber) || (b_v.type == ExcelBoolean) || (b_v.type == ExcelEmpty)) {
			if(a_v.number <= b_v.number) return FALSE;
			return TRUE;
		} 
		return FALSE;
	  case ExcelString:
	  	if(b_v.type == ExcelString) {
		  	if(strcasecmp(a_v.string,b_v.string) <= 0 ) return FALSE;
			return TRUE;	  		
		}
		return FALSE;
  	  case ExcelError:
		return a_v;
  	  case ExcelRange:
  		return NA;
  }
  return FALSE;
}

static ExcelValue more_than_or_equal(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)

	switch (a_v.type) {
  	  case ExcelNumber:
	  case ExcelBoolean: 
	  case ExcelEmpty:
		if((b_v.type == ExcelNumber) || (b_v.type == ExcelBoolean) || (b_v.type == ExcelEmpty)) {
			if(a_v.number < b_v.number) return FALSE;
			return TRUE;
		} 
		return FALSE;
	  case ExcelString:
	  	if(b_v.type == ExcelString) {
		  	if(strcasecmp(a_v.string,b_v.string) < 0 ) return FALSE;
			return TRUE;	  		
		}
		return FALSE;
  	  case ExcelError:
		return a_v;
  	  case ExcelRange:
  		return NA;
  }
  return FALSE;
}


static ExcelValue less_than(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)

	switch (a_v.type) {
  	  case ExcelNumber:
	  case ExcelBoolean: 
	  case ExcelEmpty:
		if((b_v.type == ExcelNumber) || (b_v.type == ExcelBoolean) || (b_v.type == ExcelEmpty)) {
			if(a_v.number >= b_v.number) return FALSE;
			return TRUE;
		} 
		return FALSE;
	  case ExcelString:
	  	if(b_v.type == ExcelString) {
		  	if(strcasecmp(a_v.string,b_v.string) >= 0 ) return FALSE;
			return TRUE;	  		
		}
		return FALSE;
  	  case ExcelError:
		return a_v;
  	  case ExcelRange:
  		return NA;
  }
  return FALSE;
}

static ExcelValue less_than_or_equal(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)

	switch (a_v.type) {
  	  case ExcelNumber:
	  case ExcelBoolean: 
	  case ExcelEmpty:
		if((b_v.type == ExcelNumber) || (b_v.type == ExcelBoolean) || (b_v.type == ExcelEmpty)) {
			if(a_v.number > b_v.number) return FALSE;
			return TRUE;
		} 
		return FALSE;
	  case ExcelString:
	  	if(b_v.type == ExcelString) {
		  	if(strcasecmp(a_v.string,b_v.string) > 0 ) return FALSE;
			return TRUE;	  		
		}
		return FALSE;
  	  case ExcelError:
		return a_v;
  	  case ExcelRange:
  		return NA;
  }
  return FALSE;
}

static ExcelValue subtract(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)
	NUMBER(a_v, a)
	NUMBER(b_v, b)
	CHECK_FOR_CONVERSION_ERROR
	return new_excel_number(a - b);
}

static ExcelValue multiply(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)
	NUMBER(a_v, a)
	NUMBER(b_v, b)
	CHECK_FOR_CONVERSION_ERROR
	return new_excel_number(a * b);
}

static ExcelValue sum(int array_size, ExcelValue *array) {
	double total = 0;
	int i;
	for(i=0;i<array_size;i++) {
    switch(array[i].type) {
      case ExcelNumber:
        total += array[i].number;
        break;
      case ExcelRange:
        total += number_from(sum( array[i].rows * array[i].columns, array[i].array ));
        break;
      case ExcelError:
        return array[i];
        break;
      default:
        break;
    }
	}
	return new_excel_number(total);
}

static ExcelValue max(int number_of_arguments, ExcelValue *arguments) {
	double biggest_number_found;
	int any_number_found = 0;
	int i;
	ExcelValue current_excel_value;
	
	for(i=0;i<number_of_arguments;i++) {
		current_excel_value = arguments[i];
		if(current_excel_value.type == ExcelNumber) {
			if(!any_number_found) {
				any_number_found = 1;
				biggest_number_found = current_excel_value.number;
			}
			if(current_excel_value.number > biggest_number_found) biggest_number_found = current_excel_value.number; 				
		} else if(current_excel_value.type == ExcelRange) {
			current_excel_value = max( current_excel_value.rows * current_excel_value.columns, current_excel_value.array );
			if(current_excel_value.type == ExcelError) return current_excel_value;
			if(current_excel_value.type == ExcelNumber)
				if(!any_number_found) {
					any_number_found = 1;
					biggest_number_found = current_excel_value.number;
				}
				if(current_excel_value.number > biggest_number_found) biggest_number_found = current_excel_value.number; 				
		} else if(current_excel_value.type == ExcelError) {
			return current_excel_value;
		}
	}
	if(!any_number_found) {
		any_number_found = 1;
		biggest_number_found = 0;
	}
	return new_excel_number(biggest_number_found);	
}

static ExcelValue min(int number_of_arguments, ExcelValue *arguments) {
	double smallest_number_found = 0;
	int any_number_found = 0;
	int i;
	ExcelValue current_excel_value;
	
	for(i=0;i<number_of_arguments;i++) {
		current_excel_value = arguments[i];
		if(current_excel_value.type == ExcelNumber) {
			if(!any_number_found) {
				any_number_found = 1;
				smallest_number_found = current_excel_value.number;
			}
			if(current_excel_value.number < smallest_number_found) smallest_number_found = current_excel_value.number; 				
		} else if(current_excel_value.type == ExcelRange) {
			current_excel_value = min( current_excel_value.rows * current_excel_value.columns, current_excel_value.array );
			if(current_excel_value.type == ExcelError) return current_excel_value;
			if(current_excel_value.type == ExcelNumber)
				if(!any_number_found) {
					any_number_found = 1;
					smallest_number_found = current_excel_value.number;
				}
				if(current_excel_value.number < smallest_number_found) smallest_number_found = current_excel_value.number; 				
		} else if(current_excel_value.type == ExcelError) {
			return current_excel_value;
		}
	}
	if(!any_number_found) {
		any_number_found = 1;
		smallest_number_found = 0;
	}
	return new_excel_number(smallest_number_found);	
}

static ExcelValue mod(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)
		
	NUMBER(a_v, a)
	NUMBER(b_v, b)
	CHECK_FOR_CONVERSION_ERROR
	if(b == 0) return DIV0;
	return new_excel_number(fmod(a,b));
}

static ExcelValue negative(ExcelValue a_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	NUMBER(a_v, a)
	CHECK_FOR_CONVERSION_ERROR
	return new_excel_number(-a);
}

static ExcelValue pmt(ExcelValue rate_v, ExcelValue number_of_periods_v, ExcelValue present_value_v) {
	CHECK_FOR_PASSED_ERROR(rate_v)
	CHECK_FOR_PASSED_ERROR(number_of_periods_v)
	CHECK_FOR_PASSED_ERROR(present_value_v)
		
	NUMBER(rate_v,rate)
	NUMBER(number_of_periods_v,number_of_periods)
	NUMBER(present_value_v,present_value)
	CHECK_FOR_CONVERSION_ERROR
	
	if(rate == 0) return new_excel_number(-(present_value / number_of_periods));
	return new_excel_number(-present_value*(rate*(pow((1+rate),number_of_periods)))/((pow((1+rate),number_of_periods))-1));
}

static ExcelValue power(ExcelValue a_v, ExcelValue b_v) {
	CHECK_FOR_PASSED_ERROR(a_v)
	CHECK_FOR_PASSED_ERROR(b_v)
		
	NUMBER(a_v, a)
	NUMBER(b_v, b)
	CHECK_FOR_CONVERSION_ERROR
	return new_excel_number(pow(a,b));
}

static ExcelValue excel_round(ExcelValue number_v, ExcelValue decimal_places_v) {
	CHECK_FOR_PASSED_ERROR(number_v)
	CHECK_FOR_PASSED_ERROR(decimal_places_v)
		
	NUMBER(number_v, number)
	NUMBER(decimal_places_v, decimal_places)
	CHECK_FOR_CONVERSION_ERROR
		
	double multiple = pow(10,decimal_places);
	
	return new_excel_number( round(number * multiple) / multiple );
}

static ExcelValue rounddown(ExcelValue number_v, ExcelValue decimal_places_v) {
	CHECK_FOR_PASSED_ERROR(number_v)
	CHECK_FOR_PASSED_ERROR(decimal_places_v)
		
	NUMBER(number_v, number)
	NUMBER(decimal_places_v, decimal_places)
	CHECK_FOR_CONVERSION_ERROR
		
	double multiple = pow(10,decimal_places);
	
	return new_excel_number( trunc(number * multiple) / multiple );	
}

static ExcelValue roundup(ExcelValue number_v, ExcelValue decimal_places_v) {
	CHECK_FOR_PASSED_ERROR(number_v)
	CHECK_FOR_PASSED_ERROR(decimal_places_v)
		
	NUMBER(number_v, number)
	NUMBER(decimal_places_v, decimal_places)
	CHECK_FOR_CONVERSION_ERROR
		
	double multiple = pow(10,decimal_places);
	if(number < 0) return new_excel_number( floor(number * multiple) / multiple );
	return new_excel_number( ceil(number * multiple) / multiple );	
}

static ExcelValue string_join(int number_of_arguments, ExcelValue *arguments) {
	int allocated_length = 100;
	int used_length = 0;
	char *string = malloc(allocated_length); // Freed later
	if(string == 0) {
	  printf("Out of memory");
	  exit(-1);
	}
	free_later(string);
	char *current_string;
	int current_string_length;
	int must_free_current_string;
	ExcelValue current_v;
	int i;
	for(i=0;i<number_of_arguments;i++) {
		must_free_current_string = 0;
		current_v = (ExcelValue) arguments[i];
		switch (current_v.type) {
  	  case ExcelString:
	  		current_string = current_v.string;
	  		break;
  	  case ExcelNumber:
			  current_string = malloc(20); // Freed
		  	if(current_string == 0) {
		  	  printf("Out of memory");
		  	  exit(-1);
		  	}
			must_free_current_string = 1;				  
			  snprintf(current_string,20,"%g",current_v.number);
			  break;
		  case ExcelBoolean:
		  	if(current_v.number == true) {
		  		current_string = "TRUE";
  			} else {
  				current_string = "FALSE";
  			}
        break;
		  case ExcelEmpty:
        current_string = "";
        break;
      case ExcelError:
        return current_v;
	  	case ExcelRange:
        return VALUE;
		}
		current_string_length = strlen(current_string);
		if( (used_length + current_string_length + 1) > allocated_length) {
			allocated_length += 100;
			string = realloc(string,allocated_length);
		}
		memcpy(string + used_length, current_string, current_string_length);
		if(must_free_current_string == 1) {
			free(current_string);
		}
		used_length = used_length + current_string_length;
	}
	string = realloc(string,used_length+1);
  string[used_length] = '\0';
	return new_excel_string(string);
}

static ExcelValue subtotal(ExcelValue subtotal_type_v, int number_of_arguments, ExcelValue *arguments) {
  CHECK_FOR_PASSED_ERROR(subtotal_type_v)
  NUMBER(subtotal_type_v,subtotal_type)
  CHECK_FOR_CONVERSION_ERROR
      
  switch((int) subtotal_type) {
    case 1:
    case 101:
      return average(number_of_arguments,arguments);
      break;
    case 2:
    case 102:
      return count(number_of_arguments,arguments);
      break;
    case 3:
    case 103:
      return counta(number_of_arguments,arguments);
      break;
    case 9:
    case 109:
      return sum(number_of_arguments,arguments);
      break;
    default:
      return VALUE;
      break;
  }
}

static ExcelValue sumifs(ExcelValue sum_range_v, int number_of_arguments, ExcelValue *arguments) {
  // First, set up the sum_range
  CHECK_FOR_PASSED_ERROR(sum_range_v);

  // Set up the sum range
  ExcelValue *sum_range;
  int sum_range_rows, sum_range_columns;
  
  if(sum_range_v.type == ExcelRange) {
    sum_range = sum_range_v.array;
    sum_range_rows = sum_range_v.rows;
    sum_range_columns = sum_range_v.columns;
  } else {
    sum_range = (ExcelValue*) new_excel_value_array(1);
	sum_range[0] = sum_range_v;
    sum_range_rows = 1;
    sum_range_columns = 1;
  }
  
  // Then go through and set up the check ranges
  if(number_of_arguments % 2 != 0) return VALUE;
  int number_of_criteria = number_of_arguments / 2;
  ExcelValue *criteria_range =  (ExcelValue*) new_excel_value_array(number_of_criteria);
  ExcelValue current_value;
  int i;
  for(i = 0; i < number_of_criteria; i++) {
    current_value = arguments[i*2];
    if(current_value.type == ExcelRange) {
      criteria_range[i] = current_value;
      if(current_value.rows != sum_range_rows) return VALUE;
      if(current_value.columns != sum_range_columns) return VALUE;
    } else {
      if(sum_range_rows != 1) return VALUE;
      if(sum_range_columns != 1) return VALUE;
      ExcelValue *tmp_array2 =  (ExcelValue*) new_excel_value_array(1);
      tmp_array2[0] = current_value;
      criteria_range[i] =  new_excel_range(tmp_array2,1,1);
    }
  }
  
  // Now go through and set up the criteria
  ExcelComparison *criteria =  malloc(sizeof(ExcelComparison)*number_of_criteria); // freed at end of function
  if(criteria == 0) {
	  printf("Out of memory\n");
	  exit(-1);
  }
  char *s;
  for(i = 0; i < number_of_criteria; i++) {
    current_value = arguments[(i*2)+1];
    
    if(current_value.type == ExcelString) {
      s = current_value.string;
      if(s[0] == '<') {
        if( s[1] == '>') {
          criteria[i].type = NotEqual;
          criteria[i].comparator = new_excel_string(strndup(s+2,strlen(s)-2));
        } else if(s[1] == '=') {
          criteria[i].type = LessThanOrEqual;
          criteria[i].comparator = new_excel_string(strndup(s+2,strlen(s)-2));
        } else {
          criteria[i].type = LessThan;
          criteria[i].comparator = new_excel_string(strndup(s+1,strlen(s)-1));
        }
      } else if(s[0] == '>') {
        if(s[1] == '=') {
          criteria[i].type = MoreThanOrEqual;
          criteria[i].comparator = new_excel_string(strndup(s+2,strlen(s)-2));
        } else {
          criteria[i].type = MoreThan;
          criteria[i].comparator = new_excel_string(strndup(s+1,strlen(s)-1));
        }
      } else if(s[0] == '=') {
        criteria[i].type = Equal;
        criteria[i].comparator = new_excel_string(strndup(s+1,strlen(s)-1));          
      } else {
        criteria[i].type = Equal;
        criteria[i].comparator = current_value;          
      }
    } else {
      criteria[i].type = Equal;
      criteria[i].comparator = current_value;
    }
  }
  
  double total = 0;
  int size = sum_range_columns * sum_range_rows;
  int j;
  int passed = 0;
  ExcelValue value_to_be_checked;
  ExcelComparison comparison;
  ExcelValue comparator;
  double number;
  // For each cell in the sum range
  for(j=0; j < size; j++ ) {
    passed = 1;
    for(i=0; i < number_of_criteria; i++) {
      value_to_be_checked = ((ExcelValue *) ((ExcelValue) criteria_range[i]).array)[j];
      comparison = criteria[i];
      comparator = comparison.comparator;
      
      switch(value_to_be_checked.type) {
        case ExcelError: // Errors match only errors
          if(comparison.type != Equal) passed = 0;
          if(comparator.type != ExcelError) passed = 0;
          if(value_to_be_checked.number != comparator.number) passed = 0;
          break;
        case ExcelBoolean: // Booleans match only booleans (FIXME: I think?)
          if(comparison.type != Equal) passed = 0;
          if(comparator.type != ExcelBoolean ) passed = 0;
          if(value_to_be_checked.number != comparator.number) passed = 0;
          break;
        case ExcelEmpty:
          // if(comparator.type == ExcelEmpty) break; // FIXME: Huh? In excel blank doesn't match blank?!
          if(comparator.type != ExcelString) {
            passed = 0;
            break;
          } else {
            if(strlen(comparator.string) != 0) passed = 0; // Empty strings match blanks.
            break;
          }
          break;
        case ExcelNumber:
          if(comparator.type == ExcelNumber) {
            number = comparator.number;
          } else if(comparator.type == ExcelString) {
            number = number_from(comparator);
            if(conversion_error == 1) {
              conversion_error = 0;
              passed = 0;
              break;
            }
          } else {
            passed = 0;
            break;
          }
          switch(comparison.type) {
            case Equal:
              if(value_to_be_checked.number != number) passed = 0;
              break;
            case LessThan:
              if(value_to_be_checked.number >= number) passed = 0;
              break;            
            case LessThanOrEqual:
              if(value_to_be_checked.number > number) passed = 0;
              break;                        
            case NotEqual:
              if(value_to_be_checked.number == number) passed = 0;
              break;            
            case MoreThanOrEqual:
              if(value_to_be_checked.number < number) passed = 0;
              break;            
            case MoreThan:
              if(value_to_be_checked.number <= number) passed = 0;
              break;
          }
          break;
        case ExcelString:
          // First case, the comparator is a number, simplification is that it can only be equal
          if(comparator.type == ExcelNumber) {
            if(comparison.type != Equal) {
              printf("This shouldn't be possible?");
              passed = 0;
              break;
            }
            number = number_from(value_to_be_checked);
            if(conversion_error == 1) {
              conversion_error = 0;
              passed = 0;
              break;
            }
            if(number != comparator.number) {
              passed = 0;
              break;
            } else {
              break;
            }
          // Second case, the comparator is also a string, so need to be able to do full range of tests
          } else if(comparator.type == ExcelString) {
            switch(comparison.type) {
              case Equal:
                if(excel_equal(value_to_be_checked,comparator).number == 0) passed = 0;
                break;
              case LessThan:
                if(less_than(value_to_be_checked,comparator).number == 0) passed = 0;
                break;            
              case LessThanOrEqual:
                if(less_than_or_equal(value_to_be_checked,comparator).number == 0) passed = 0;
                break;                        
              case NotEqual:
                if(not_equal(value_to_be_checked,comparator).number == 0) passed = 0;
                break;            
              case MoreThanOrEqual:
                if(more_than_or_equal(value_to_be_checked,comparator).number == 0) passed = 0;
                break;            
              case MoreThan:
                if(more_than(value_to_be_checked,comparator).number == 0) passed = 0;
                break;
              }
          } else {
            passed = 0;
            break;
          }
          break;
        case ExcelRange:
          return VALUE;            
      }
      if(passed == 0) break;
    }
    if(passed == 1) {
      current_value = sum_range[j];
      if(current_value.type == ExcelError) {
        return current_value;
      } else if(current_value.type == ExcelNumber) {
        total += current_value.number;
      }
    }
  }
  // Tidy up
  free(criteria);
  return new_excel_number(total);
}

static ExcelValue sumif(ExcelValue check_range_v, ExcelValue criteria_v, ExcelValue sum_range_v ) {
	ExcelValue tmp_array_sumif[] = {check_range_v, criteria_v};
	return sumifs(sum_range_v,2,tmp_array_sumif);
}

static ExcelValue sumif_2(ExcelValue check_range_v, ExcelValue criteria_v) {
	ExcelValue tmp_array_sumif2[] = {check_range_v, criteria_v};
	return sumifs(check_range_v,2,tmp_array_sumif2);
}

static ExcelValue sumproduct(int number_of_arguments, ExcelValue *arguments) {
  if(number_of_arguments <1) return VALUE;
  
  int a;
  int i;
  int j;
  int rows;
  int columns;
  ExcelValue current_value;
  ExcelValue **ranges = malloc(sizeof(ExcelValue *)*number_of_arguments); // Added free statements
  if(ranges == 0) {
	  printf("Out of memory\n");
	  exit(-1);
  }
  double product = 1;
  double sum = 0;
  
  // Find out dimensions of first argument
  if(arguments[0].type == ExcelRange) {
    rows = arguments[0].rows;
    columns = arguments[0].columns;
  } else {
    rows = 1;
    columns = 1;
  }
  // Extract arrays from each of the given ranges, checking for errors and bounds as we go
  for(a=0;a<number_of_arguments;a++) {
    current_value = arguments[a];
    switch(current_value.type) {
      case ExcelRange:
        if(current_value.rows != rows || current_value.columns != columns) return VALUE;
        ranges[a] = current_value.array;
        break;
      case ExcelError:
		free(ranges);
        return current_value;
        break;
      case ExcelEmpty:
		free(ranges);
        return VALUE;
        break;
      default:
        if(rows != 1 && columns !=1) return VALUE;
        ranges[a] = (ExcelValue*) new_excel_value_array(1);
        ranges[a][0] = arguments[a];
        break;
    }
  }
  	
	for(i=0;i<rows;i++) {
		for(j=0;j<columns;j++) {
			product = 1;
			for(a=0;a<number_of_arguments;a++) {
				current_value = ranges[a][(i*columns)+j];
				if(current_value.type == ExcelNumber) {
					product *= current_value.number;
				} else {
					product *= 0;
				}
			}
			sum += product;
		}
	}
	free(ranges);
  	return new_excel_number(sum);
}

static ExcelValue vlookup_3(ExcelValue lookup_value_v,ExcelValue lookup_table_v, ExcelValue column_number_v) {
  return vlookup(lookup_value_v,lookup_table_v,column_number_v,TRUE);
}

static ExcelValue vlookup(ExcelValue lookup_value_v,ExcelValue lookup_table_v, ExcelValue column_number_v, ExcelValue match_type_v) {
  CHECK_FOR_PASSED_ERROR(lookup_value_v)
  CHECK_FOR_PASSED_ERROR(lookup_table_v)
  CHECK_FOR_PASSED_ERROR(column_number_v)
  CHECK_FOR_PASSED_ERROR(match_type_v)

  if(lookup_value_v.type == ExcelEmpty) return NA;
  if(lookup_table_v.type != ExcelRange) return NA;
  if(column_number_v.type != ExcelNumber) return NA;
  if(match_type_v.type != ExcelBoolean) return NA;
    
  int i;
  int last_good_match = 0;
  int rows = lookup_table_v.rows;
  int columns = lookup_table_v.columns;
  ExcelValue *array = lookup_table_v.array;
  ExcelValue possible_match_v;
  
  if(column_number_v.number > columns) return REF;
  if(column_number_v.number < 1) return VALUE;
  
  if(match_type_v.number == false) { // Exact match required
    for(i=0; i< rows; i++) {
      possible_match_v = array[i*columns];
      if(excel_equal(lookup_value_v,possible_match_v).number == true) {
        return array[(i*columns)+(((int) column_number_v.number) - 1)];
      }
    }
    return NA;
  } else { // Highest value that is less than or equal
    for(i=0; i< rows; i++) {
      possible_match_v = array[i*columns];
      if(lookup_value_v.type != possible_match_v.type) continue;
      if(more_than(possible_match_v,lookup_value_v).number == true) {
        if(i == 0) return NA;
        return array[((i-1)*columns)+(((int) column_number_v.number) - 1)];
      } else {
        last_good_match = i;
      }
    }
    return array[(last_good_match*columns)+(((int) column_number_v.number) - 1)];   
  }
  return NA;
}



int test_functions() {
	// Test ABS
	assert(excel_abs(ONE).number == 1);
	assert(excel_abs(new_excel_number(-1)).number == 1);
	assert(excel_abs(VALUE).type == ExcelError);
	
	// Test ADD
	assert(add(ONE,new_excel_number(-2.5)).number == -1.5);
	assert(add(ONE,VALUE).type == ExcelError);
	
	// Test AND
	ExcelValue true_array1[] = { TRUE, new_excel_number(10)};
	ExcelValue true_array2[] = { ONE };
	ExcelValue false_array1[] = { FALSE, new_excel_number(10)};
	ExcelValue false_array2[] = { TRUE, new_excel_number(0)};
	// ExcelValue error_array1[] = { new_excel_number(10)}; // Not implemented
	ExcelValue error_array2[] = { TRUE, NA};
	assert(excel_and(2,true_array1).number == 1);
	assert(excel_and(1,true_array2).number == 1);
	assert(excel_and(2,false_array1).number == 0);
	assert(excel_and(2,false_array2).number == 0);
	// assert(excel_and(1,error_array1).type == ExcelError); // Not implemented
	assert(excel_and(2,error_array2).type == ExcelError);
	
	// Test AVERAGE
	ExcelValue array1[] = { new_excel_number(10), new_excel_number(5), TRUE, FALSE};
	ExcelValue array1_v = new_excel_range(array1,2,2);
	ExcelValue array2[] = { array1_v, new_excel_number(9), new_excel_string("Hello")};
	ExcelValue array3[] = { array1_v, new_excel_number(9), new_excel_string("Hello"), VALUE};
	assert(average(4, array1).number == 7.5);
	assert(average(3, array2).number == 8);
	assert(average(4, array3).type == ExcelError);
	
	// Test CHOOSE
	assert(choose(ONE,4,array1).number == 10);
	assert(choose(new_excel_number(4),4,array1).type == ExcelBoolean);
	assert(choose(new_excel_number(0),4,array1).type == ExcelError);
	assert(choose(new_excel_number(5),4,array1).type == ExcelError);
	assert(choose(ONE,4,array3).type == ExcelError);	
	
	// Test COUNT
	assert(count(4,array1).number == 2);
	assert(count(3,array2).number == 3);
	assert(count(4,array3).number == 3);
	
	// Test COUNTA
	ExcelValue count_a_test_array_1[] = { new_excel_number(10), new_excel_number(5), TRUE, FALSE, new_excel_string("Hello"), VALUE, BLANK};
  ExcelValue count_a_test_array_1_v = new_excel_range(count_a_test_array_1,7,1);
  ExcelValue count_a_test_array_2[] = {new_excel_string("Bye"),count_a_test_array_1_v};
	assert(counta(7, count_a_test_array_1).number == 6);
  assert(counta(2, count_a_test_array_2).number == 7);
	
	// Test divide
	assert(divide(new_excel_number(12.4),new_excel_number(3.2)).number == 3.875);
	assert(divide(new_excel_number(12.4),new_excel_number(0)).type == ExcelError);
	
	// Test excel_equal
	assert(excel_equal(new_excel_number(1.2),new_excel_number(3.4)).type == ExcelBoolean);
	assert(excel_equal(new_excel_number(1.2),new_excel_number(3.4)).number == false);
	assert(excel_equal(new_excel_number(1.2),new_excel_number(1.2)).number == true);
	assert(excel_equal(new_excel_string("hello"), new_excel_string("HELLO")).number == true);
	assert(excel_equal(new_excel_string("hello world"), new_excel_string("HELLO")).number == false);
	assert(excel_equal(new_excel_string("1"), ONE).number == false);
	assert(excel_equal(DIV0, ONE).type == ExcelError);

	// Test not_equal
	assert(not_equal(new_excel_number(1.2),new_excel_number(3.4)).type == ExcelBoolean);
	assert(not_equal(new_excel_number(1.2),new_excel_number(3.4)).number == true);
	assert(not_equal(new_excel_number(1.2),new_excel_number(1.2)).number == false);
	assert(not_equal(new_excel_string("hello"), new_excel_string("HELLO")).number == false);
	assert(not_equal(new_excel_string("hello world"), new_excel_string("HELLO")).number == true);
	assert(not_equal(new_excel_string("1"), ONE).number == true);
	assert(not_equal(DIV0, ONE).type == ExcelError);
	
	// Test excel_if
	// Two argument version
	assert(excel_if_2(TRUE,new_excel_number(10)).type == ExcelNumber);
	assert(excel_if_2(TRUE,new_excel_number(10)).number == 10);
	assert(excel_if_2(FALSE,new_excel_number(10)).type == ExcelBoolean);
	assert(excel_if_2(FALSE,new_excel_number(10)).number == false);
	assert(excel_if_2(NA,new_excel_number(10)).type == ExcelError);
	// Three argument version
	assert(excel_if(TRUE,new_excel_number(10),new_excel_number(20)).type == ExcelNumber);
	assert(excel_if(TRUE,new_excel_number(10),new_excel_number(20)).number == 10);
	assert(excel_if(FALSE,new_excel_number(10),new_excel_number(20)).type == ExcelNumber);
	assert(excel_if(FALSE,new_excel_number(10),new_excel_number(20)).number == 20);
	assert(excel_if(NA,new_excel_number(10),new_excel_number(20)).type == ExcelError);
	
	// Test excel_match
	ExcelValue excel_match_array_1[] = { new_excel_number(10), new_excel_number(100) };
	ExcelValue excel_match_array_1_v = new_excel_range(excel_match_array_1,1,2);
	ExcelValue excel_match_array_2[] = { new_excel_string("Pear"), new_excel_string("Bear"), new_excel_string("Apple") };
	ExcelValue excel_match_array_2_v = new_excel_range(excel_match_array_2,3,1);
	ExcelValue excel_match_array_4[] = { ONE, BLANK, new_excel_number(0) };
	ExcelValue excel_match_array_4_v = new_excel_range(excel_match_array_4,1,3);
	ExcelValue excel_match_array_5[] = { ONE, new_excel_number(0), BLANK };
	ExcelValue excel_match_array_5_v = new_excel_range(excel_match_array_5,1,3);
	
	// Two argument version
	assert(excel_match_2(new_excel_number(10),excel_match_array_1_v).number == 1);
	assert(excel_match_2(new_excel_number(100),excel_match_array_1_v).number == 2);
	assert(excel_match_2(new_excel_number(1000),excel_match_array_1_v).type == ExcelError);
    assert(excel_match_2(new_excel_number(0), excel_match_array_4_v).number == 2);
    assert(excel_match_2(BLANK, excel_match_array_5_v).number == 2);

	// Three argument version	
    assert(excel_match(new_excel_number(10.0), excel_match_array_1_v, new_excel_number(0) ).number == 1);
    assert(excel_match(new_excel_number(100.0), excel_match_array_1_v, new_excel_number(0) ).number == 2);
    assert(excel_match(new_excel_number(1000.0), excel_match_array_1_v, new_excel_number(0) ).type == ExcelError);
    assert(excel_match(new_excel_string("bEAr"), excel_match_array_2_v, new_excel_number(0) ).number == 2);
    assert(excel_match(new_excel_number(1000.0), excel_match_array_1_v, ONE ).number == 2);
    assert(excel_match(new_excel_number(1.0), excel_match_array_1_v, ONE ).type == ExcelError);
    assert(excel_match(new_excel_string("Care"), excel_match_array_2_v, new_excel_number(-1) ).number == 1  );
    assert(excel_match(new_excel_string("Zebra"), excel_match_array_2_v, new_excel_number(-1) ).type == ExcelError);
    assert(excel_match(new_excel_string("a"), excel_match_array_2_v, new_excel_number(-1) ).number == 2);
	
	// When not given a range
    assert(excel_match(new_excel_number(10.0), new_excel_number(10), new_excel_number(0.0)).number == 1);
    assert(excel_match(new_excel_number(20.0), new_excel_number(10), new_excel_number(0.0)).type == ExcelError);
    assert(excel_match(new_excel_number(10.0), excel_match_array_1_v, BLANK).number == 1);

	// Test more than on
	// .. numbers
    assert(more_than(ONE,new_excel_number(2)).number == false);
    assert(more_than(ONE,ONE).number == false);
    assert(more_than(ONE,new_excel_number(0)).number == true);
	// .. booleans
    assert(more_than(FALSE,FALSE).number == false);
    assert(more_than(FALSE,TRUE).number == false);
    assert(more_than(TRUE,FALSE).number == true);
    assert(more_than(TRUE,TRUE).number == false);
	// ..strings
    assert(more_than(new_excel_string("HELLO"),new_excel_string("Ardvark")).number == true);		
    assert(more_than(new_excel_string("HELLO"),new_excel_string("world")).number == false);
    assert(more_than(new_excel_string("HELLO"),new_excel_string("hello")).number == false);
	// ..blanks
    assert(more_than(BLANK,ONE).number == false);
    assert(more_than(BLANK,new_excel_number(-1)).number == true);
    assert(more_than(ONE,BLANK).number == true);
    assert(more_than(new_excel_number(-1),BLANK).number == false);

	// Test less than on
	// .. numbers
    assert(less_than(ONE,new_excel_number(2)).number == true);
    assert(less_than(ONE,ONE).number == false);
    assert(less_than(ONE,new_excel_number(0)).number == false);
	// .. booleans
    assert(less_than(FALSE,FALSE).number == false);
    assert(less_than(FALSE,TRUE).number == true);
    assert(less_than(TRUE,FALSE).number == false);
    assert(less_than(TRUE,TRUE).number == false);
	// ..strings
    assert(less_than(new_excel_string("HELLO"),new_excel_string("Ardvark")).number == false);		
    assert(less_than(new_excel_string("HELLO"),new_excel_string("world")).number == true);
    assert(less_than(new_excel_string("HELLO"),new_excel_string("hello")).number == false);
	// ..blanks
    assert(less_than(BLANK,ONE).number == true);
    assert(less_than(BLANK,new_excel_number(-1)).number == false);
    assert(less_than(ONE,BLANK).number == false);
    assert(less_than(new_excel_number(-1),BLANK).number == true);

	// Test FIND function
	// ... should find the first occurrence of one string in another, returning :value if the string doesn't match
	assert(find_2(new_excel_string("one"),new_excel_string("onetwothree")).number == 1);
	assert(find_2(new_excel_string("one"),new_excel_string("twoonethree")).number == 4);
	assert(find_2(new_excel_string("one"),new_excel_string("twoonthree")).type == ExcelError);
    // ... should find the first occurrence of one string in another after a given index, returning :value if the string doesn't match
	assert(find(new_excel_string("one"),new_excel_string("onetwothree"),ONE).number == 1);
	assert(find(new_excel_string("one"),new_excel_string("twoonethree"),new_excel_number(5)).type == ExcelError);
	assert(find(new_excel_string("one"),new_excel_string("oneone"),new_excel_number(2)).number == 4);
    // ... should be possible for the start_num to be a string, if that string converts to a number
	assert(find(new_excel_string("one"),new_excel_string("oneone"),new_excel_string("2")).number == 4);
    // ... should return a :value error when given anything but a number as the third argument
	assert(find(new_excel_string("one"),new_excel_string("oneone"),new_excel_string("a")).type == ExcelError);
    // ... should return a :value error when given a third argument that is less than 1 or greater than the length of the string
	assert(find(new_excel_string("one"),new_excel_string("oneone"),new_excel_number(0)).type == ExcelError);
	assert(find(new_excel_string("one"),new_excel_string("oneone"),new_excel_number(-1)).type == ExcelError);
	assert(find(new_excel_string("one"),new_excel_string("oneone"),new_excel_number(7)).type == ExcelError);
	// ... BLANK in the first argument matches any character
	assert(find_2(BLANK,new_excel_string("abcdefg")).number == 1);
	assert(find(BLANK,new_excel_string("abcdefg"),new_excel_number(4)).number == 4);
    // ... should treat BLANK in the second argument as an empty string
	assert(find_2(BLANK,BLANK).number == 1);
	assert(find_2(new_excel_string("a"),BLANK).type == ExcelError);
	// ... should return an error if any argument is an error
	assert(find(new_excel_string("one"),new_excel_string("onetwothree"),NA).type == ExcelError);
	assert(find(new_excel_string("one"),NA,ONE).type == ExcelError);
	assert(find(NA,new_excel_string("onetwothree"),ONE).type == ExcelError);
	
	// Test the IFERROR function
    assert(iferror(new_excel_string("ok"),ONE).type == ExcelString);
	assert(iferror(VALUE,ONE).type == ExcelNumber);		
	
	// Test the INDEX function
	ExcelValue index_array_1[] = { new_excel_number(10), new_excel_number(20), BLANK };
	ExcelValue index_array_1_v_column = new_excel_range(index_array_1,3,1);
	ExcelValue index_array_1_v_row = new_excel_range(index_array_1,1,3);
	ExcelValue index_array_2[] = { BLANK, ONE, new_excel_number(10), new_excel_number(11), new_excel_number(100), new_excel_number(101) };
	ExcelValue index_array_2_v = new_excel_range(index_array_2,3,2);
	// ... if given one argument should return the value at that offset in the range
	assert(excel_index_2(index_array_1_v_column,new_excel_number(2.0)).number == 20);
	assert(excel_index_2(index_array_1_v_row,new_excel_number(2.0)).number == 20);
	// ... but not if the range is not a single row or single column
	assert(excel_index_2(index_array_2_v,new_excel_number(2.0)).type == ExcelError);
    // ... it should return the value in the array at position row_number, column_number
	assert(excel_index(new_excel_number(10),ONE,ONE).number == 10);
	assert(excel_index(index_array_2_v,new_excel_number(1.0),new_excel_number(2.0)).number == 1);
	assert(excel_index(index_array_2_v,new_excel_number(2.0),new_excel_number(1.0)).number == 10);
	assert(excel_index(index_array_2_v,new_excel_number(3.0),new_excel_number(1.0)).number == 100);
	assert(excel_index(index_array_2_v,new_excel_number(3.0),new_excel_number(3.0)).type == ExcelError);
	// ... it should return ZERO not blank, if a blank cell is picked
	assert(excel_index(index_array_2_v,new_excel_number(1.0),new_excel_number(1.0)).type == ExcelNumber);
	assert(excel_index(index_array_2_v,new_excel_number(1.0),new_excel_number(1.0)).number == 0);
	assert(excel_index_2(index_array_1_v_row,new_excel_number(3.0)).type == ExcelNumber);
	assert(excel_index_2(index_array_1_v_row,new_excel_number(3.0)).number == 0);
	// ... it should return the whole row if given a zero column number
	ExcelValue index_result_1_v = excel_index(index_array_2_v,new_excel_number(1.0),new_excel_number(0.0));
	assert(index_result_1_v.type == ExcelRange);
	assert(index_result_1_v.rows == 1);
	assert(index_result_1_v.columns == 2);
	ExcelValue *index_result_1_a = index_result_1_v.array;
	assert(index_result_1_a[0].number == 0);
	assert(index_result_1_a[1].number == 1);
	// ... it should return the whole column if given a zero row number
	ExcelValue index_result_2_v = excel_index(index_array_2_v,new_excel_number(0),new_excel_number(1.0));
	assert(index_result_2_v.type == ExcelRange);
	assert(index_result_2_v.rows == 3);
	assert(index_result_2_v.columns == 1);
	ExcelValue *index_result_2_a = index_result_2_v.array;
	assert(index_result_2_a[0].number == 0);
	assert(index_result_2_a[1].number == 10);
	assert(index_result_2_a[2].number == 100);
    // ... it should return a :ref error when given arguments outside array range
	assert(excel_index_2(index_array_1_v_row,new_excel_number(-1)).type == ExcelError);
	assert(excel_index_2(index_array_1_v_row,new_excel_number(4)).type == ExcelError);
    // ... it should treat BLANK as zero if given as a required row or column number
	assert(excel_index(index_array_2_v,new_excel_number(1.0),BLANK).type == ExcelRange);
	assert(excel_index(index_array_2_v,BLANK,new_excel_number(2.0)).type == ExcelRange);
    // ... it should return an error if an argument is an error
	assert(excel_index(NA,NA,NA).type == ExcelError);
	
	// LEFT(string,[characters])
	// ... should return the left n characters from a string
    assert(strcmp(left_1(new_excel_string("ONE")).string,"O") == 0);
    assert(strcmp(left(new_excel_string("ONE"),ONE).string,"O") == 0);
    assert(strcmp(left(new_excel_string("ONE"),new_excel_number(3)).string,"ONE") == 0);
	// ... should turn numbers into strings before processing
	assert(strcmp(left(new_excel_number(1.31e12),new_excel_number(3)).string, "131") == 0);
	// ... should turn booleans into the words TRUE and FALSE before processing
    assert(strcmp(left(TRUE,new_excel_number(3)).string,"TRU") == 0);
	assert(strcmp(left(FALSE,new_excel_number(3)).string,"FAL") == 0);
	// ... should return BLANK if given BLANK for either argument
	assert(left(BLANK,new_excel_number(3)).type == ExcelEmpty);
	assert(left(new_excel_string("ONE"),BLANK).type == ExcelEmpty);
	// ... should return an error if an argument is an error
    assert(left_1(NA).type == ExcelError);
    assert(left(new_excel_string("ONE"),NA).type == ExcelError);
	
	// Test less than or equal to
	// .. numbers
    assert(less_than_or_equal(ONE,new_excel_number(2)).number == true);
    assert(less_than_or_equal(ONE,ONE).number == true);
    assert(less_than_or_equal(ONE,new_excel_number(0)).number == false);
	// .. booleans
    assert(less_than_or_equal(FALSE,FALSE).number == true);
    assert(less_than_or_equal(FALSE,TRUE).number == true);
    assert(less_than_or_equal(TRUE,FALSE).number == false);
    assert(less_than_or_equal(TRUE,TRUE).number == true);
	// ..strings
    assert(less_than_or_equal(new_excel_string("HELLO"),new_excel_string("Ardvark")).number == false);		
    assert(less_than_or_equal(new_excel_string("HELLO"),new_excel_string("world")).number == true);
    assert(less_than_or_equal(new_excel_string("HELLO"),new_excel_string("hello")).number == true);
	// ..blanks
    assert(less_than_or_equal(BLANK,ONE).number == true);
    assert(less_than_or_equal(BLANK,new_excel_number(-1)).number == false);
    assert(less_than_or_equal(ONE,BLANK).number == false);
    assert(less_than_or_equal(new_excel_number(-1),BLANK).number == true);

	// Test MAX
	assert(max(4, array1).number == 10);
	assert(max(3, array2).number == 10);
	assert(max(4, array3).type == ExcelError);

	// Test MIN
	assert(min(4, array1).number == 5);
	assert(min(3, array2).number == 5);
	assert(min(4, array3).type == ExcelError);

	// Test MOD
    // ... should return the remainder of a number
	assert(mod(new_excel_number(10), new_excel_number(3)).number == 1.0);
	assert(mod(new_excel_number(10), new_excel_number(5)).number == 0.0);
    // ... should be possible for the the arguments to be strings, if they convert to a number
	assert(mod(new_excel_string("3.5"),new_excel_string("2")).number == 1.5);
    // ... should treat BLANK as zero
	assert(mod(BLANK,new_excel_number(10)).number == 0);
	assert(mod(new_excel_number(10),BLANK).type == ExcelError);
	assert(mod(BLANK,BLANK).type == ExcelError);
    // ... should treat true as 1 and FALSE as 0
	assert((mod(new_excel_number(1.1),TRUE).number - 0.1) < 0.001);	
	assert(mod(new_excel_number(1.1),FALSE).type == ExcelError);
	assert(mod(FALSE,new_excel_number(10)).number == 0);
    // ... should return an error when given inappropriate arguments
	assert(mod(new_excel_string("Asdasddf"),new_excel_string("adsfads")).type == ExcelError);
    // ... should return an error if an argument is an error
	assert(mod(new_excel_number(1),VALUE).type == ExcelError);
	assert(mod(VALUE,new_excel_number(1)).type == ExcelError);
	assert(mod(VALUE,VALUE).type == ExcelError);
	
	// Test more than or equal to on
	// .. numbers
    assert(more_than_or_equal(ONE,new_excel_number(2)).number == false);
    assert(more_than_or_equal(ONE,ONE).number == true);
    assert(more_than_or_equal(ONE,new_excel_number(0)).number == true);
	// .. booleans
    assert(more_than_or_equal(FALSE,FALSE).number == true);
    assert(more_than_or_equal(FALSE,TRUE).number == false);
    assert(more_than_or_equal(TRUE,FALSE).number == true);
    assert(more_than_or_equal(TRUE,TRUE).number == true);
	// ..strings
    assert(more_than_or_equal(new_excel_string("HELLO"),new_excel_string("Ardvark")).number == true);		
    assert(more_than_or_equal(new_excel_string("HELLO"),new_excel_string("world")).number == false);
    assert(more_than_or_equal(new_excel_string("HELLO"),new_excel_string("hello")).number == true);
	// ..blanks
    assert(more_than_or_equal(BLANK,BLANK).number == true);
    assert(more_than_or_equal(BLANK,ONE).number == false);
    assert(more_than_or_equal(BLANK,new_excel_number(-1)).number == true);
    assert(more_than_or_equal(ONE,BLANK).number == true);
    assert(more_than_or_equal(new_excel_number(-1),BLANK).number == false);	
	
	// Test negative
    // ... should return the negative of its arguments
	assert(negative(new_excel_number(1)).number == -1);
	assert(negative(new_excel_number(-1)).number == 1);
    // ... should treat strings that only contain numbers as numbers
	assert(negative(new_excel_string("10")).number == -10);
	assert(negative(new_excel_string("-1.3")).number == 1.3);
    // ... should return an error when given inappropriate arguments
	assert(negative(new_excel_string("Asdasddf")).type == ExcelError);
    // ... should treat BLANK as zero
	assert(negative(BLANK).number == 0);
	
	// Test PMT(rate,number_of_periods,present_value) - optional arguments not yet implemented
    // ... should calculate the monthly payment required for a given principal, interest rate and loan period
    assert((pmt(new_excel_number(0.1),new_excel_number(10),new_excel_number(100)).number - -16.27) < 0.01);
    assert((pmt(new_excel_number(0.0123),new_excel_number(99.1),new_excel_number(123.32)).number - -2.159) < 0.01);
    assert((pmt(new_excel_number(0),new_excel_number(2),new_excel_number(10)).number - -5) < 0.01);

	// Test power
    // ... should return sum of its arguments
	assert(power(new_excel_number(2),new_excel_number(3)).number == 8);
	assert(power(new_excel_number(4.0),new_excel_number(0.5)).number == 2.0);
	
	// Test round
    assert(excel_round(new_excel_number(1.1), new_excel_number(0)).number == 1.0);
    assert(excel_round(new_excel_number(1.5), new_excel_number(0)).number == 2.0);
    assert(excel_round(new_excel_number(1.56),new_excel_number(1)).number == 1.6);
    assert(excel_round(new_excel_number(-1.56),new_excel_number(1)).number == -1.6);

	// Test rounddown
    assert(rounddown(new_excel_number(1.1), new_excel_number(0)).number == 1.0);
    assert(rounddown(new_excel_number(1.5), new_excel_number(0)).number == 1.0);
    assert(rounddown(new_excel_number(1.56),new_excel_number(1)).number == 1.5);
    assert(rounddown(new_excel_number(-1.56),new_excel_number(1)).number == -1.5);	

	// Test roundup
    assert(roundup(new_excel_number(1.1), new_excel_number(0)).number == 2.0);
    assert(roundup(new_excel_number(1.5), new_excel_number(0)).number == 2.0);
    assert(roundup(new_excel_number(1.56),new_excel_number(1)).number == 1.6);
    assert(roundup(new_excel_number(-1.56),new_excel_number(1)).number == -1.6);	
	
	// Test string joining
	ExcelValue string_join_array_1[] = {new_excel_string("Hello "), new_excel_string("world")};
	ExcelValue string_join_array_2[] = {new_excel_string("Hello "), new_excel_string("world"), new_excel_string("!")};
	ExcelValue string_join_array_3[] = {new_excel_string("Top "), new_excel_number(10.0)};
	ExcelValue string_join_array_4[] = {new_excel_string("Top "), new_excel_number(10.5)};	
	ExcelValue string_join_array_5[] = {new_excel_string("Top "), TRUE, FALSE};	
	// ... should return a string by combining its arguments
	// inspect_excel_value(string_join(2, string_join_array_1));
  assert(string_join(2, string_join_array_1).string[6] == 'w');
  assert(string_join(2, string_join_array_1).string[11] == '\0');
	// ... should cope with an arbitrary number of arguments
  assert(string_join(3, string_join_array_2).string[11] == '!');
  assert(string_join(3, string_join_array_3).string[12] == '\0');
	// ... should convert values to strings as it goes
  assert(string_join(2, string_join_array_3).string[4] == '1');
  assert(string_join(2, string_join_array_3).string[5] == '0');
  assert(string_join(2, string_join_array_3).string[6] == '\0');
	// ... should convert integer values into strings without decimal points, and float values with decimal points
  assert(string_join(2, string_join_array_4).string[4] == '1');
  assert(string_join(2, string_join_array_4).string[5] == '0');
  assert(string_join(2, string_join_array_4).string[6] == '.');
  assert(string_join(2, string_join_array_4).string[7] == '5');
  assert(string_join(2, string_join_array_4).string[8] == '\0');
	// ... should convert TRUE and FALSE into strings
  assert(string_join(3,string_join_array_5).string[4] == 'T');
	
  // Test SUBTOTAL function
  ExcelValue subtotal_array_1[] = {new_excel_number(10),new_excel_number(100),BLANK};
  ExcelValue subtotal_array_1_v = new_excel_range(subtotal_array_1,3,1);
  ExcelValue subtotal_array_2[] = {new_excel_number(1),new_excel_string("two"),subtotal_array_1_v};
  
  // new_excel_number(1.0); 
  // inspect_excel_value(new_excel_number(1.0)); 
  // inspect_excel_value(new_excel_range(subtotal_array_2,3,1)); 
  // inspect_excel_value(subtotal(new_excel_number(1.0),3,subtotal_array_2)); 
  
  assert(subtotal(new_excel_number(1.0),3,subtotal_array_2).number == 111.0/3.0);
  assert(subtotal(new_excel_number(2.0),3,subtotal_array_2).number == 3);
  assert(subtotal(new_excel_number(3.0),7, count_a_test_array_1).number == 6);
  assert(subtotal(new_excel_number(3.0),3,subtotal_array_2).number == 4);
  assert(subtotal(new_excel_number(9.0),3,subtotal_array_2).number == 111);
  assert(subtotal(new_excel_number(101.0),3,subtotal_array_2).number == 111.0/3.0);
  assert(subtotal(new_excel_number(102.0),3,subtotal_array_2).number == 3);
  assert(subtotal(new_excel_number(103.0),3,subtotal_array_2).number == 4);
  assert(subtotal(new_excel_number(109.0),3,subtotal_array_2).number == 111);
  
  // Test SUMIFS function
  ExcelValue sumifs_array_1[] = {new_excel_number(10),new_excel_number(100),BLANK};
  ExcelValue sumifs_array_1_v = new_excel_range(sumifs_array_1,3,1);
  ExcelValue sumifs_array_2[] = {new_excel_string("pear"),new_excel_string("bear"),new_excel_string("apple")};
  ExcelValue sumifs_array_2_v = new_excel_range(sumifs_array_2,3,1);
  ExcelValue sumifs_array_3[] = {new_excel_number(1),new_excel_number(2),new_excel_number(3),new_excel_number(4),new_excel_number(5),new_excel_number(5)};
  ExcelValue sumifs_array_3_v = new_excel_range(sumifs_array_3,6,1);
  ExcelValue sumifs_array_4[] = {new_excel_string("CO2"),new_excel_string("CH4"),new_excel_string("N2O"),new_excel_string("CH4"),new_excel_string("N2O"),new_excel_string("CO2")};
  ExcelValue sumifs_array_4_v = new_excel_range(sumifs_array_4,6,1);
  ExcelValue sumifs_array_5[] = {new_excel_string("1A"),new_excel_string("1A"),new_excel_string("1A"),new_excel_number(4),new_excel_number(4),new_excel_number(5)};
  ExcelValue sumifs_array_5_v = new_excel_range(sumifs_array_5,6,1);
  
  // ... should only sum values that meet all of the criteria
  ExcelValue sumifs_array_6[] = { sumifs_array_1_v, new_excel_number(10), sumifs_array_2_v, new_excel_string("Bear") };
  assert(sumifs(sumifs_array_1_v,4,sumifs_array_6).number == 0.0);
  
  ExcelValue sumifs_array_7[] = { sumifs_array_1_v, new_excel_number(10), sumifs_array_2_v, new_excel_string("Pear") };
  assert(sumifs(sumifs_array_1_v,4,sumifs_array_7).number == 10.0);
  
  // ... should work when single cells are given where ranges expected
  ExcelValue sumifs_array_8[] = { new_excel_string("CAR"), new_excel_string("CAR"), new_excel_string("FCV"), new_excel_string("FCV")};
  assert(sumifs(new_excel_number(0.143897265452564), 4, sumifs_array_8).number == 0.143897265452564);

  // ... should match numbers with strings that contain numbers
  ExcelValue sumifs_array_9[] = { new_excel_number(10), new_excel_string("10.0")};
  assert(sumifs(new_excel_number(100),2,sumifs_array_9).number == 100);
  
  ExcelValue sumifs_array_10[] = { sumifs_array_4_v, new_excel_string("CO2"), sumifs_array_5_v, new_excel_number(2)};
  assert(sumifs(sumifs_array_3_v,4, sumifs_array_10).number == 0);
  
  // ... should match with strings that contain criteria
  ExcelValue sumifs_array_10a[] = { sumifs_array_3_v, new_excel_string("=5")};
  assert(sumifs(sumifs_array_3_v,2, sumifs_array_10a).number == 10);

  ExcelValue sumifs_array_10b[] = { sumifs_array_3_v, new_excel_string("<>3")};
  assert(sumifs(sumifs_array_3_v,2, sumifs_array_10b).number == 17);

  ExcelValue sumifs_array_10c[] = { sumifs_array_3_v, new_excel_string("<3")};
  assert(sumifs(sumifs_array_3_v,2, sumifs_array_10c).number == 3);
  
  ExcelValue sumifs_array_10d[] = { sumifs_array_3_v, new_excel_string("<=3")};
  assert(sumifs(sumifs_array_3_v,2, sumifs_array_10d).number == 6);

  ExcelValue sumifs_array_10e[] = { sumifs_array_3_v, new_excel_string(">3")};
  assert(sumifs(sumifs_array_3_v,2, sumifs_array_10e).number == 14);

  ExcelValue sumifs_array_10f[] = { sumifs_array_3_v, new_excel_string(">=3")};
  assert(sumifs(sumifs_array_3_v,2, sumifs_array_10f).number == 17);
  
  // ... should treat BLANK as an empty string when in the check_range, but not in the criteria
  ExcelValue sumifs_array_11[] = { BLANK, new_excel_number(20)};
  assert(sumifs(new_excel_number(100),2,sumifs_array_11).number == 0);
  
  ExcelValue sumifs_array_12[] = {BLANK, new_excel_string("")};
  assert(sumifs(new_excel_number(100),2,sumifs_array_12).number == 100);
  
  ExcelValue sumifs_array_13[] = {BLANK, BLANK};
  assert(sumifs(new_excel_number(100),2,sumifs_array_13).number == 0);
    
  // ... should return an error if range argument is an error
  assert(sumifs(REF,2,sumifs_array_13).type == ExcelError);
  
  
  // Test SUMIF
  // ... where there is only a check range
  assert(sumif_2(sumifs_array_1_v,new_excel_string(">0")).number == 110.0);
  assert(sumif_2(sumifs_array_1_v,new_excel_string(">10")).number == 100.0);
  assert(sumif_2(sumifs_array_1_v,new_excel_string("<100")).number == 10.0);
  
  // ... where there is a seprate sum range
  ExcelValue sumif_array_1[] = {new_excel_number(15),new_excel_number(20), new_excel_number(30)};
  ExcelValue sumif_array_1_v = new_excel_range(sumif_array_1,3,1);
  assert(sumif(sumifs_array_1_v,new_excel_string("10"),sumif_array_1_v).number == 15);
  
  
  // Test SUMPRODUCT
  ExcelValue sumproduct_1[] = { new_excel_number(10), new_excel_number(100), BLANK};
  ExcelValue sumproduct_2[] = { BLANK, new_excel_number(100), new_excel_number(10), BLANK};
  ExcelValue sumproduct_3[] = { BLANK };
  ExcelValue sumproduct_4[] = { new_excel_number(10), new_excel_number(100), new_excel_number(1000)};
  ExcelValue sumproduct_5[] = { new_excel_number(1), new_excel_number(2), new_excel_number(3)};
  ExcelValue sumproduct_6[] = { new_excel_number(1), new_excel_number(2), new_excel_number(4), new_excel_number(5)};
  ExcelValue sumproduct_7[] = { new_excel_number(10), new_excel_number(20), new_excel_number(40), new_excel_number(50)};
  ExcelValue sumproduct_8[] = { new_excel_number(11), new_excel_number(21), new_excel_number(41), new_excel_number(51)};
  ExcelValue sumproduct_9[] = { BLANK, BLANK };
  
  ExcelValue sumproduct_1_v = new_excel_range( sumproduct_1, 3, 1);
  ExcelValue sumproduct_2_v = new_excel_range( sumproduct_2, 3, 1);
  ExcelValue sumproduct_3_v = new_excel_range( sumproduct_3, 1, 1);
  // ExcelValue sumproduct_4_v = new_excel_range( sumproduct_4, 1, 3); // Unused
  ExcelValue sumproduct_5_v = new_excel_range( sumproduct_5, 3, 1);
  ExcelValue sumproduct_6_v = new_excel_range( sumproduct_6, 2, 2);
  ExcelValue sumproduct_7_v = new_excel_range( sumproduct_7, 2, 2);
  ExcelValue sumproduct_8_v = new_excel_range( sumproduct_8, 2, 2);
  ExcelValue sumproduct_9_v = new_excel_range( sumproduct_9, 2, 1);
  
  // ... should multiply together and then sum the elements in row or column areas given as arguments
  ExcelValue sumproducta_1[] = {sumproduct_1_v, sumproduct_2_v};
  assert(sumproduct(2,sumproducta_1).number == 100*100);

  // ... should return :value when miss-matched array sizes
  ExcelValue sumproducta_2[] = {sumproduct_1_v, sumproduct_3_v};
  assert(sumproduct(2,sumproducta_2).type == ExcelError);

  // ... if all its arguments are single values, should multiply them together
  // ExcelValue *sumproducta_3 = sumproduct_4;
  assert(sumproduct(3,sumproduct_4).number == 10*100*1000);

  // ... if it only has one range as an argument, should add its elements together
  ExcelValue sumproducta_4[] = {sumproduct_5_v};
  assert(sumproduct(1,sumproducta_4).number == 1 + 2 + 3);

  // ... if given multi row and column areas as arguments, should multipy the corresponding cell in each area and then add them all
  ExcelValue sumproducta_5[] = {sumproduct_6_v, sumproduct_7_v, sumproduct_8_v};
  assert(sumproduct(3,sumproducta_5).number == 1*10*11 + 2*20*21 + 4*40*41 + 5*50*51);

  // ... should raise an error if BLANK values outside of an array
  ExcelValue sumproducta_6[] = {BLANK,new_excel_number(1)};
  assert(sumproduct(2,sumproducta_6).type == ExcelError);

  // ... should ignore non-numeric values within an array
  ExcelValue sumproducta_7[] = {sumproduct_9_v, sumproduct_9_v};
  assert(sumproduct(2,sumproducta_7).number == 0);

  // ... should return an error if an argument is an error
  ExcelValue sumproducta_8[] = {VALUE};
  assert(sumproduct(1,sumproducta_8).type == ExcelError);
  
  // Test VLOOKUP
  ExcelValue vlookup_a1[] = {new_excel_number(1),new_excel_number(10),new_excel_number(2),new_excel_number(20),new_excel_number(3),new_excel_number(30)};
  ExcelValue vlookup_a2[] = {new_excel_string("hello"),new_excel_number(10),new_excel_number(2),new_excel_number(20),new_excel_number(3),new_excel_number(30)};
  ExcelValue vlookup_a3[] = {BLANK,new_excel_number(10),new_excel_number(2),new_excel_number(20),new_excel_number(3),new_excel_number(30)};
  ExcelValue vlookup_a1_v = new_excel_range(vlookup_a1,3,2);
  ExcelValue vlookup_a2_v = new_excel_range(vlookup_a2,3,2);
  ExcelValue vlookup_a3_v = new_excel_range(vlookup_a3,3,2);
  // ... should match the first argument against the first column of the table in the second argument, returning the value in the column specified by the third argument
  assert(vlookup_3(new_excel_number(2.0),vlookup_a1_v,new_excel_number(2)).number == 20);
  assert(vlookup_3(new_excel_number(1.5),vlookup_a1_v,new_excel_number(2)).number == 10);
  assert(vlookup_3(new_excel_number(0.5),vlookup_a1_v,new_excel_number(2)).type == ExcelError);
  assert(vlookup_3(new_excel_number(10),vlookup_a1_v,new_excel_number(2)).number == 30);
  assert(vlookup_3(new_excel_number(2.6),vlookup_a1_v,new_excel_number(2)).number == 20);
  // ... has a four argument variant that matches the lookup type
  assert(vlookup(new_excel_number(2.6),vlookup_a1_v,new_excel_number(2),TRUE).number == 20);
  assert(vlookup(new_excel_number(2.6),vlookup_a1_v,new_excel_number(2),FALSE).type == ExcelError);
  assert(vlookup(new_excel_string("HELLO"),vlookup_a2_v,new_excel_number(2),FALSE).number == 10);
  assert(vlookup(new_excel_string("HELMP"),vlookup_a2_v,new_excel_number(2),TRUE).number == 10);
  // ... BLANK should not match with anything" do
  assert(vlookup_3(BLANK,vlookup_a3_v,new_excel_number(2)).type == ExcelError);
  // ... should return an error if an argument is an error" do
  assert(vlookup(VALUE,vlookup_a1_v,new_excel_number(2),FALSE).type == ExcelError);
  assert(vlookup(new_excel_number(2.0),VALUE,new_excel_number(2),FALSE).type == ExcelError);
  assert(vlookup(new_excel_number(2.0),vlookup_a1_v,VALUE,FALSE).type == ExcelError);
  assert(vlookup(new_excel_number(2.0),vlookup_a1_v,new_excel_number(2),VALUE).type == ExcelError);
  assert(vlookup(VALUE,VALUE,VALUE,VALUE).type == ExcelError);
	
  // Test SUM
  ExcelValue sum_array_0[] = {new_excel_number(1084.4557258064517),new_excel_number(32.0516914516129),new_excel_number(137.36439193548387)};
  ExcelValue sum_array_0_v = new_excel_range(sum_array_0,3,1);
  ExcelValue sum_array_1[] = {sum_array_0_v};
  assert(sum(1,sum_array_1).number == 1253.8718091935484);
  
  // Release memory
  free_all_allocated_memory();
  
  return 0;
}

int main() {
	return test_functions();
}
// End of the generic c functions

// Start of the file specific functions

// definitions
static ExcelValue _common0();
static ExcelValue _common1();
static ExcelValue _common2();
static ExcelValue _common3();
static ExcelValue _common4();
static ExcelValue _common5();
static ExcelValue _common6();
ExcelValue valuetypes_a1();
ExcelValue valuetypes_a2();
ExcelValue valuetypes_a3();
ExcelValue valuetypes_a4();
ExcelValue valuetypes_a5();
ExcelValue valuetypes_a6();
ExcelValue formulaetypes_a1();
ExcelValue formulaetypes_b1();
ExcelValue formulaetypes_a2();
ExcelValue formulaetypes_b2();
ExcelValue formulaetypes_a3();
ExcelValue formulaetypes_b3();
ExcelValue formulaetypes_a4();
ExcelValue formulaetypes_b4();
ExcelValue formulaetypes_a5();
ExcelValue formulaetypes_b5();
ExcelValue formulaetypes_a6();
ExcelValue formulaetypes_b6();
ExcelValue formulaetypes_a7();
ExcelValue formulaetypes_b7();
ExcelValue formulaetypes_a8();
ExcelValue formulaetypes_b8();
ExcelValue ranges_b1();
ExcelValue ranges_c1();
ExcelValue ranges_a2();
ExcelValue ranges_b2();
ExcelValue ranges_c2();
ExcelValue ranges_a3();
ExcelValue ranges_b3();
ExcelValue ranges_c3();
ExcelValue ranges_a4();
ExcelValue ranges_b4();
ExcelValue ranges_c4();
ExcelValue ranges_f4();
ExcelValue ranges_e5();
ExcelValue ranges_f5();
ExcelValue ranges_g5();
ExcelValue ranges_f6();
ExcelValue referencing_a1();
ExcelValue referencing_a2();
ExcelValue referencing_a4();
ExcelValue referencing_b4();
ExcelValue referencing_c4();
ExcelValue referencing_a5();
ExcelValue referencing_b8();
ExcelValue referencing_b9();
ExcelValue referencing_b11();
ExcelValue referencing_c11();
ExcelValue tables_a1();
ExcelValue tables_b2();
ExcelValue tables_c2();
ExcelValue tables_d2();
ExcelValue tables_b3();
ExcelValue tables_c3();
ExcelValue tables_d3();
ExcelValue tables_b4();
ExcelValue tables_c4();
ExcelValue tables_d4();
ExcelValue tables_f4();
ExcelValue tables_g4();
ExcelValue tables_h4();
ExcelValue tables_b5();
ExcelValue tables_c5();
ExcelValue tables_e6();
ExcelValue tables_f6();
ExcelValue tables_g6();
ExcelValue tables_e7();
ExcelValue tables_f7();
ExcelValue tables_g7();
ExcelValue tables_e8();
ExcelValue tables_f8();
ExcelValue tables_g8();
ExcelValue tables_e9();
ExcelValue tables_f9();
ExcelValue tables_g9();
ExcelValue tables_c10();
ExcelValue tables_e10();
ExcelValue tables_f10();
ExcelValue tables_g10();
ExcelValue tables_c11();
ExcelValue tables_e11();
ExcelValue tables_f11();
ExcelValue tables_g11();
ExcelValue tables_c12();
ExcelValue tables_c13();
ExcelValue tables_c14();
ExcelValue s_innapropriate_sheet_name__c4();
// end of definitions

// Used to decide whether to recalculate a cell
static int variable_set[94];

// Used to reset all cached values and free up memory
void reset() {
  int i;
  cell_counter = 0;
  free_all_allocated_memory(); 
  for(i = 0; i < 94; i++) {
    variable_set[i] = 0;
  }
};

// starting the value constants
static ExcelValue C1 = {.type = ExcelString, .string = "Hello"};
static ExcelValue C2 = {.type = ExcelNumber, .number = 1};
static ExcelValue C3 = {.type = ExcelNumber, .number = 3.1415};
static ExcelValue C4 = {.type = ExcelString, .string = "Simple"};
static ExcelValue C5 = {.type = ExcelNumber, .number = 2};
static ExcelValue C6 = {.type = ExcelString, .string = "Sharing"};
static ExcelValue C7 = {.type = ExcelNumber, .number = 267.7467614837482};
static ExcelValue C8 = {.type = ExcelString, .string = "Shared"};
static ExcelValue C9 = {.type = ExcelString, .string = "Array (single)"};
static ExcelValue C10 = {.type = ExcelString, .string = "Arraying (multiple)"};
static ExcelValue C11 = {.type = ExcelString, .string = "Not Eight"};
static ExcelValue C12 = {.type = ExcelString, .string = "Arrayed (multiple)"};
static ExcelValue C13 = {.type = ExcelString, .string = "This sheet"};
static ExcelValue C14 = {.type = ExcelString, .string = "Other sheet"};
static ExcelValue C15 = {.type = ExcelString, .string = "Standard"};
static ExcelValue C16 = {.type = ExcelString, .string = "Column"};
static ExcelValue C17 = {.type = ExcelString, .string = "Row"};
static ExcelValue C18 = {.type = ExcelNumber, .number = 3};
static ExcelValue C19 = {.type = ExcelNumber, .number = 10};
static ExcelValue C20 = {.type = ExcelString, .string = "Named"};
static ExcelValue C21 = {.type = ExcelString, .string = "Reference"};
static ExcelValue C22 = {.type = ExcelString, .string = "ColA"};
static ExcelValue C23 = {.type = ExcelString, .string = "ColB"};
static ExcelValue C24 = {.type = ExcelString, .string = "Column1"};
static ExcelValue C25 = {.type = ExcelString, .string = "A"};
static ExcelValue C26 = {.type = ExcelString, .string = "B"};
static ExcelValue C27 = {.type = ExcelString, .string = "2B"};
// ending the value constants

// starting common elements
static ExcelValue _common0() {
  static ExcelValue result;
  if(variable_set[0] == 1) { return result;}
  result = add(referencing_a4(),C2);
  variable_set[0] = 1;
  return result;
}

static ExcelValue _common1() {
  static ExcelValue result;
  if(variable_set[1] == 1) { return result;}
  static ExcelValue array1[3];
  array1[0] = tables_b5();
  array1[1] = tables_c5();
  array1[2] = BLANK;
  ExcelValue array1_ev = new_excel_range(array1,1,3);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[1] = 1;
  return result;
}

static ExcelValue _common2() {
  static ExcelValue result;
  if(variable_set[2] == 1) { return result;}
  static ExcelValue array0[3];
  array0[0] = tables_b5();
  array0[1] = tables_c5();
  array0[2] = BLANK;
  ExcelValue array0_ev = new_excel_range(array0,1,3);
  result = array0_ev;
  variable_set[2] = 1;
  return result;
}

static ExcelValue _common3() {
  static ExcelValue result;
  if(variable_set[3] == 1) { return result;}
  static ExcelValue array1[2];
  array1[0] = tables_b3();
  array1[1] = tables_b4();
  ExcelValue array1_ev = new_excel_range(array1,2,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[3] = 1;
  return result;
}

static ExcelValue _common4() {
  static ExcelValue result;
  if(variable_set[4] == 1) { return result;}
  static ExcelValue array0[2];
  array0[0] = tables_b3();
  array0[1] = tables_b4();
  ExcelValue array0_ev = new_excel_range(array0,2,1);
  result = array0_ev;
  variable_set[4] = 1;
  return result;
}

static ExcelValue _common5() {
  static ExcelValue result;
  if(variable_set[5] == 1) { return result;}
  static ExcelValue array1[6];
  array1[0] = tables_b3();
  array1[1] = tables_c3();
  array1[2] = tables_d3();
  array1[3] = tables_b4();
  array1[4] = tables_c4();
  array1[5] = tables_d4();
  ExcelValue array1_ev = new_excel_range(array1,2,3);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[5] = 1;
  return result;
}

static ExcelValue _common6() {
  static ExcelValue result;
  if(variable_set[6] == 1) { return result;}
  static ExcelValue array0[6];
  array0[0] = tables_b3();
  array0[1] = tables_c3();
  array0[2] = tables_d3();
  array0[3] = tables_b4();
  array0[4] = tables_c4();
  array0[5] = tables_d4();
  ExcelValue array0_ev = new_excel_range(array0,2,3);
  result = array0_ev;
  variable_set[6] = 1;
  return result;
}

// ending common elements

// start ValueTypes
ExcelValue valuetypes_a1_default() {
  return TRUE;
}
static ExcelValue valuetypes_a1_variable;
ExcelValue valuetypes_a1() { if(variable_set[7] == 1) { return valuetypes_a1_variable; } else { return valuetypes_a1_default(); } }
void set_valuetypes_a1(ExcelValue newValue) { variable_set[7] = 1; valuetypes_a1_variable = newValue; }

ExcelValue valuetypes_a2_default() {
  return C1;
}
static ExcelValue valuetypes_a2_variable;
ExcelValue valuetypes_a2() { if(variable_set[8] == 1) { return valuetypes_a2_variable; } else { return valuetypes_a2_default(); } }
void set_valuetypes_a2(ExcelValue newValue) { variable_set[8] = 1; valuetypes_a2_variable = newValue; }

ExcelValue valuetypes_a3_default() {
  return C2;
}
static ExcelValue valuetypes_a3_variable;
ExcelValue valuetypes_a3() { if(variable_set[9] == 1) { return valuetypes_a3_variable; } else { return valuetypes_a3_default(); } }
void set_valuetypes_a3(ExcelValue newValue) { variable_set[9] = 1; valuetypes_a3_variable = newValue; }

ExcelValue valuetypes_a4_default() {
  return C3;
}
static ExcelValue valuetypes_a4_variable;
ExcelValue valuetypes_a4() { if(variable_set[10] == 1) { return valuetypes_a4_variable; } else { return valuetypes_a4_default(); } }
void set_valuetypes_a4(ExcelValue newValue) { variable_set[10] = 1; valuetypes_a4_variable = newValue; }

ExcelValue valuetypes_a5_default() {
  return NAME;
}
static ExcelValue valuetypes_a5_variable;
ExcelValue valuetypes_a5() { if(variable_set[11] == 1) { return valuetypes_a5_variable; } else { return valuetypes_a5_default(); } }
void set_valuetypes_a5(ExcelValue newValue) { variable_set[11] = 1; valuetypes_a5_variable = newValue; }

ExcelValue valuetypes_a6_default() {
  return C1;
}
static ExcelValue valuetypes_a6_variable;
ExcelValue valuetypes_a6() { if(variable_set[12] == 1) { return valuetypes_a6_variable; } else { return valuetypes_a6_default(); } }
void set_valuetypes_a6(ExcelValue newValue) { variable_set[12] = 1; valuetypes_a6_variable = newValue; }

// end ValueTypes

// start FormulaeTypes
ExcelValue formulaetypes_a1() {
  static ExcelValue result;
  if(variable_set[13] == 1) { return result;}
  result = C4;
  variable_set[13] = 1;
  return result;
}

ExcelValue formulaetypes_b1() {
  static ExcelValue result;
  if(variable_set[14] == 1) { return result;}
  result = C5;
  variable_set[14] = 1;
  return result;
}

ExcelValue formulaetypes_a2() {
  static ExcelValue result;
  if(variable_set[15] == 1) { return result;}
  result = C6;
  variable_set[15] = 1;
  return result;
}

ExcelValue formulaetypes_b2() {
  static ExcelValue result;
  if(variable_set[16] == 1) { return result;}
  result = C7;
  variable_set[16] = 1;
  return result;
}

ExcelValue formulaetypes_a3() {
  static ExcelValue result;
  if(variable_set[17] == 1) { return result;}
  result = C8;
  variable_set[17] = 1;
  return result;
}

ExcelValue formulaetypes_b3() {
  static ExcelValue result;
  if(variable_set[18] == 1) { return result;}
  result = C7;
  variable_set[18] = 1;
  return result;
}

ExcelValue formulaetypes_a4() {
  static ExcelValue result;
  if(variable_set[19] == 1) { return result;}
  result = C8;
  variable_set[19] = 1;
  return result;
}

ExcelValue formulaetypes_b4() {
  static ExcelValue result;
  if(variable_set[20] == 1) { return result;}
  result = C7;
  variable_set[20] = 1;
  return result;
}

ExcelValue formulaetypes_a5() {
  static ExcelValue result;
  if(variable_set[21] == 1) { return result;}
  result = C9;
  variable_set[21] = 1;
  return result;
}

ExcelValue formulaetypes_b5() {
  static ExcelValue result;
  if(variable_set[22] == 1) { return result;}
  result = C5;
  variable_set[22] = 1;
  return result;
}

ExcelValue formulaetypes_a6() {
  static ExcelValue result;
  if(variable_set[23] == 1) { return result;}
  result = C10;
  variable_set[23] = 1;
  return result;
}

ExcelValue formulaetypes_b6() {
  static ExcelValue result;
  if(variable_set[24] == 1) { return result;}
  result = C11;
  variable_set[24] = 1;
  return result;
}

ExcelValue formulaetypes_a7() {
  static ExcelValue result;
  if(variable_set[25] == 1) { return result;}
  result = C12;
  variable_set[25] = 1;
  return result;
}

ExcelValue formulaetypes_b7() {
  static ExcelValue result;
  if(variable_set[26] == 1) { return result;}
  result = C11;
  variable_set[26] = 1;
  return result;
}

ExcelValue formulaetypes_a8() {
  static ExcelValue result;
  if(variable_set[27] == 1) { return result;}
  result = C12;
  variable_set[27] = 1;
  return result;
}

ExcelValue formulaetypes_b8() {
  static ExcelValue result;
  if(variable_set[28] == 1) { return result;}
  result = C11;
  variable_set[28] = 1;
  return result;
}

// end FormulaeTypes

// start Ranges
ExcelValue ranges_b1() {
  static ExcelValue result;
  if(variable_set[29] == 1) { return result;}
  result = C13;
  variable_set[29] = 1;
  return result;
}

ExcelValue ranges_c1() {
  static ExcelValue result;
  if(variable_set[30] == 1) { return result;}
  result = C14;
  variable_set[30] = 1;
  return result;
}

ExcelValue ranges_a2() {
  static ExcelValue result;
  if(variable_set[31] == 1) { return result;}
  result = C15;
  variable_set[31] = 1;
  return result;
}

ExcelValue ranges_b2() {
  static ExcelValue result;
  if(variable_set[32] == 1) { return result;}
  static ExcelValue array1[3];
  array1[0] = ranges_f4();
  array1[1] = ranges_f5();
  array1[2] = ranges_f6();
  ExcelValue array1_ev = new_excel_range(array1,3,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[32] = 1;
  return result;
}

ExcelValue ranges_c2() {
  static ExcelValue result;
  if(variable_set[33] == 1) { return result;}
  static ExcelValue array1[2];
  array1[0] = valuetypes_a3();
  array1[1] = valuetypes_a4();
  ExcelValue array1_ev = new_excel_range(array1,2,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[33] = 1;
  return result;
}

ExcelValue ranges_a3() {
  static ExcelValue result;
  if(variable_set[34] == 1) { return result;}
  result = C16;
  variable_set[34] = 1;
  return result;
}

ExcelValue ranges_b3() {
  static ExcelValue result;
  if(variable_set[35] == 1) { return result;}
  static ExcelValue array1[6];
  array1[0] = BLANK;
  array1[1] = BLANK;
  array1[2] = BLANK;
  array1[3] = ranges_f4();
  array1[4] = ranges_f5();
  array1[5] = ranges_f6();
  ExcelValue array1_ev = new_excel_range(array1,6,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[35] = 1;
  return result;
}

ExcelValue ranges_c3() {
  static ExcelValue result;
  if(variable_set[36] == 1) { return result;}
  static ExcelValue array1[6];
  array1[0] = valuetypes_a1();
  array1[1] = valuetypes_a2();
  array1[2] = valuetypes_a3();
  array1[3] = valuetypes_a4();
  array1[4] = valuetypes_a5();
  array1[5] = valuetypes_a6();
  ExcelValue array1_ev = new_excel_range(array1,6,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[36] = 1;
  return result;
}

ExcelValue ranges_a4() {
  static ExcelValue result;
  if(variable_set[37] == 1) { return result;}
  result = C17;
  variable_set[37] = 1;
  return result;
}

ExcelValue ranges_b4() {
  static ExcelValue result;
  if(variable_set[38] == 1) { return result;}
  static ExcelValue array1[7];
  array1[0] = BLANK;
  array1[1] = BLANK;
  array1[2] = BLANK;
  array1[3] = BLANK;
  array1[4] = ranges_e5();
  array1[5] = ranges_f5();
  array1[6] = ranges_g5();
  ExcelValue array1_ev = new_excel_range(array1,1,7);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[38] = 1;
  return result;
}

ExcelValue ranges_c4() {
  static ExcelValue result;
  if(variable_set[39] == 1) { return result;}
  static ExcelValue array1[1];
  array1[0] = valuetypes_a4();
  ExcelValue array1_ev = new_excel_range(array1,1,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[39] = 1;
  return result;
}

ExcelValue ranges_f4_default() {
  return C2;
}
static ExcelValue ranges_f4_variable;
ExcelValue ranges_f4() { if(variable_set[40] == 1) { return ranges_f4_variable; } else { return ranges_f4_default(); } }
void set_ranges_f4(ExcelValue newValue) { variable_set[40] = 1; ranges_f4_variable = newValue; }

ExcelValue ranges_e5_default() {
  return C2;
}
static ExcelValue ranges_e5_variable;
ExcelValue ranges_e5() { if(variable_set[41] == 1) { return ranges_e5_variable; } else { return ranges_e5_default(); } }
void set_ranges_e5(ExcelValue newValue) { variable_set[41] = 1; ranges_e5_variable = newValue; }

ExcelValue ranges_f5_default() {
  return C5;
}
static ExcelValue ranges_f5_variable;
ExcelValue ranges_f5() { if(variable_set[42] == 1) { return ranges_f5_variable; } else { return ranges_f5_default(); } }
void set_ranges_f5(ExcelValue newValue) { variable_set[42] = 1; ranges_f5_variable = newValue; }

ExcelValue ranges_g5_default() {
  return C18;
}
static ExcelValue ranges_g5_variable;
ExcelValue ranges_g5() { if(variable_set[43] == 1) { return ranges_g5_variable; } else { return ranges_g5_default(); } }
void set_ranges_g5(ExcelValue newValue) { variable_set[43] = 1; ranges_g5_variable = newValue; }

ExcelValue ranges_f6_default() {
  return C18;
}
static ExcelValue ranges_f6_variable;
ExcelValue ranges_f6() { if(variable_set[44] == 1) { return ranges_f6_variable; } else { return ranges_f6_default(); } }
void set_ranges_f6(ExcelValue newValue) { variable_set[44] = 1; ranges_f6_variable = newValue; }

// end Ranges

// start Referencing
ExcelValue referencing_a1() {
  static ExcelValue result;
  if(variable_set[45] == 1) { return result;}
  result = referencing_c4();
  variable_set[45] = 1;
  return result;
}

ExcelValue referencing_a2() {
  static ExcelValue result;
  if(variable_set[46] == 1) { return result;}
  result = referencing_c4();
  variable_set[46] = 1;
  return result;
}

ExcelValue referencing_a4_default() {
  return C19;
}
static ExcelValue referencing_a4_variable;
ExcelValue referencing_a4() { if(variable_set[47] == 1) { return referencing_a4_variable; } else { return referencing_a4_default(); } }
void set_referencing_a4(ExcelValue newValue) { variable_set[47] = 1; referencing_a4_variable = newValue; }

ExcelValue referencing_b4() {
  static ExcelValue result;
  if(variable_set[48] == 1) { return result;}
  result = _common0();
  variable_set[48] = 1;
  return result;
}

ExcelValue referencing_c4() {
  static ExcelValue result;
  if(variable_set[49] == 1) { return result;}
  result = add(_common0(),C2);
  variable_set[49] = 1;
  return result;
}

ExcelValue referencing_a5() {
  static ExcelValue result;
  if(variable_set[50] == 1) { return result;}
  result = C18;
  variable_set[50] = 1;
  return result;
}

ExcelValue referencing_b8() {
  static ExcelValue result;
  if(variable_set[51] == 1) { return result;}
  result = referencing_c4();
  variable_set[51] = 1;
  return result;
}

ExcelValue referencing_b9() {
  static ExcelValue result;
  if(variable_set[52] == 1) { return result;}
  result = _common1();
  variable_set[52] = 1;
  return result;
}

ExcelValue referencing_b11() {
  static ExcelValue result;
  if(variable_set[53] == 1) { return result;}
  result = C20;
  variable_set[53] = 1;
  return result;
}

ExcelValue referencing_c11() {
  static ExcelValue result;
  if(variable_set[54] == 1) { return result;}
  result = C21;
  variable_set[54] = 1;
  return result;
}

// end Referencing

// start Tables
ExcelValue tables_a1() {
  static ExcelValue result;
  if(variable_set[55] == 1) { return result;}
  result = BLANK;
  variable_set[55] = 1;
  return result;
}

ExcelValue tables_b2_default() {
  return C22;
}
static ExcelValue tables_b2_variable;
ExcelValue tables_b2() { if(variable_set[56] == 1) { return tables_b2_variable; } else { return tables_b2_default(); } }
void set_tables_b2(ExcelValue newValue) { variable_set[56] = 1; tables_b2_variable = newValue; }

ExcelValue tables_c2_default() {
  return C23;
}
static ExcelValue tables_c2_variable;
ExcelValue tables_c2() { if(variable_set[57] == 1) { return tables_c2_variable; } else { return tables_c2_default(); } }
void set_tables_c2(ExcelValue newValue) { variable_set[57] = 1; tables_c2_variable = newValue; }

ExcelValue tables_d2_default() {
  return C24;
}
static ExcelValue tables_d2_variable;
ExcelValue tables_d2() { if(variable_set[58] == 1) { return tables_d2_variable; } else { return tables_d2_default(); } }
void set_tables_d2(ExcelValue newValue) { variable_set[58] = 1; tables_d2_variable = newValue; }

ExcelValue tables_b3_default() {
  return C2;
}
static ExcelValue tables_b3_variable;
ExcelValue tables_b3() { if(variable_set[59] == 1) { return tables_b3_variable; } else { return tables_b3_default(); } }
void set_tables_b3(ExcelValue newValue) { variable_set[59] = 1; tables_b3_variable = newValue; }

ExcelValue tables_c3_default() {
  return C25;
}
static ExcelValue tables_c3_variable;
ExcelValue tables_c3() { if(variable_set[60] == 1) { return tables_c3_variable; } else { return tables_c3_default(); } }
void set_tables_c3(ExcelValue newValue) { variable_set[60] = 1; tables_c3_variable = newValue; }

ExcelValue tables_d3() {
  static ExcelValue result;
  if(variable_set[61] == 1) { return result;}
  ExcelValue array0[] = {tables_b3(),tables_c3()};
  result = string_join(2, array0);
  variable_set[61] = 1;
  return result;
}

ExcelValue tables_b4_default() {
  return C5;
}
static ExcelValue tables_b4_variable;
ExcelValue tables_b4() { if(variable_set[62] == 1) { return tables_b4_variable; } else { return tables_b4_default(); } }
void set_tables_b4(ExcelValue newValue) { variable_set[62] = 1; tables_b4_variable = newValue; }

ExcelValue tables_c4_default() {
  return C26;
}
static ExcelValue tables_c4_variable;
ExcelValue tables_c4() { if(variable_set[63] == 1) { return tables_c4_variable; } else { return tables_c4_default(); } }
void set_tables_c4(ExcelValue newValue) { variable_set[63] = 1; tables_c4_variable = newValue; }

ExcelValue tables_d4() {
  static ExcelValue result;
  if(variable_set[64] == 1) { return result;}
  ExcelValue array0[] = {tables_b4(),tables_c4()};
  result = string_join(2, array0);
  variable_set[64] = 1;
  return result;
}

ExcelValue tables_f4() {
  static ExcelValue result;
  if(variable_set[65] == 1) { return result;}
  result = tables_c4();
  variable_set[65] = 1;
  return result;
}

ExcelValue tables_g4() {
  static ExcelValue result;
  if(variable_set[66] == 1) { return result;}
  static ExcelValue array0[3];
  array0[0] = tables_b4();
  array0[1] = tables_c4();
  array0[2] = tables_d4();
  ExcelValue array0_ev = new_excel_range(array0,1,3);
  result = excel_match(C27,array0_ev,FALSE);
  variable_set[66] = 1;
  return result;
}

ExcelValue tables_h4() {
  static ExcelValue result;
  if(variable_set[67] == 1) { return result;}
  static ExcelValue array0[2];
  array0[0] = tables_c4();
  array0[1] = tables_d4();
  ExcelValue array0_ev = new_excel_range(array0,1,2);
  result = excel_match_2(C26,array0_ev);
  variable_set[67] = 1;
  return result;
}

ExcelValue tables_b5() {
  static ExcelValue result;
  if(variable_set[68] == 1) { return result;}
  result = _common3();
  variable_set[68] = 1;
  return result;
}

ExcelValue tables_c5() {
  static ExcelValue result;
  if(variable_set[69] == 1) { return result;}
  static ExcelValue array1[2];
  array1[0] = tables_c3();
  array1[1] = tables_c4();
  ExcelValue array1_ev = new_excel_range(array1,2,1);
  ExcelValue array0[] = {array1_ev};
  result = sum(1, array0);
  variable_set[69] = 1;
  return result;
}

ExcelValue tables_e6() {
  static ExcelValue result;
  if(variable_set[70] == 1) { return result;}
  result = tables_b2();
  variable_set[70] = 1;
  return result;
}

ExcelValue tables_f6() {
  static ExcelValue result;
  if(variable_set[71] == 1) { return result;}
  result = tables_c2();
  variable_set[71] = 1;
  return result;
}

ExcelValue tables_g6() {
  static ExcelValue result;
  if(variable_set[72] == 1) { return result;}
  result = tables_d2();
  variable_set[72] = 1;
  return result;
}

ExcelValue tables_e7() {
  static ExcelValue result;
  if(variable_set[73] == 1) { return result;}
  result = tables_b5();
  variable_set[73] = 1;
  return result;
}

ExcelValue tables_f7() {
  static ExcelValue result;
  if(variable_set[74] == 1) { return result;}
  result = tables_c5();
  variable_set[74] = 1;
  return result;
}

ExcelValue tables_g7() {
  static ExcelValue result;
  if(variable_set[75] == 1) { return result;}
  result = BLANK;
  variable_set[75] = 1;
  return result;
}

ExcelValue tables_e8() {
  static ExcelValue result;
  if(variable_set[76] == 1) { return result;}
  result = tables_b2();
  variable_set[76] = 1;
  return result;
}

ExcelValue tables_f8() {
  static ExcelValue result;
  if(variable_set[77] == 1) { return result;}
  result = tables_c2();
  variable_set[77] = 1;
  return result;
}

ExcelValue tables_g8() {
  static ExcelValue result;
  if(variable_set[78] == 1) { return result;}
  result = tables_d2();
  variable_set[78] = 1;
  return result;
}

ExcelValue tables_e9() {
  static ExcelValue result;
  if(variable_set[79] == 1) { return result;}
  result = tables_b3();
  variable_set[79] = 1;
  return result;
}

ExcelValue tables_f9() {
  static ExcelValue result;
  if(variable_set[80] == 1) { return result;}
  result = tables_c3();
  variable_set[80] = 1;
  return result;
}

ExcelValue tables_g9() {
  static ExcelValue result;
  if(variable_set[81] == 1) { return result;}
  result = tables_d3();
  variable_set[81] = 1;
  return result;
}

ExcelValue tables_c10() {
  static ExcelValue result;
  if(variable_set[82] == 1) { return result;}
  result = _common1();
  variable_set[82] = 1;
  return result;
}

ExcelValue tables_e10() {
  static ExcelValue result;
  if(variable_set[83] == 1) { return result;}
  result = tables_b4();
  variable_set[83] = 1;
  return result;
}

ExcelValue tables_f10() {
  static ExcelValue result;
  if(variable_set[84] == 1) { return result;}
  result = tables_c4();
  variable_set[84] = 1;
  return result;
}

ExcelValue tables_g10() {
  static ExcelValue result;
  if(variable_set[85] == 1) { return result;}
  result = tables_d4();
  variable_set[85] = 1;
  return result;
}

ExcelValue tables_c11() {
  static ExcelValue result;
  if(variable_set[86] == 1) { return result;}
  result = _common3();
  variable_set[86] = 1;
  return result;
}

ExcelValue tables_e11() {
  static ExcelValue result;
  if(variable_set[87] == 1) { return result;}
  result = tables_b5();
  variable_set[87] = 1;
  return result;
}

ExcelValue tables_f11() {
  static ExcelValue result;
  if(variable_set[88] == 1) { return result;}
  result = tables_c5();
  variable_set[88] = 1;
  return result;
}

ExcelValue tables_g11() {
  static ExcelValue result;
  if(variable_set[89] == 1) { return result;}
  result = BLANK;
  variable_set[89] = 1;
  return result;
}

ExcelValue tables_c12() {
  static ExcelValue result;
  if(variable_set[90] == 1) { return result;}
  result = tables_b5();
  variable_set[90] = 1;
  return result;
}

ExcelValue tables_c13() {
  static ExcelValue result;
  if(variable_set[91] == 1) { return result;}
  result = _common5();
  variable_set[91] = 1;
  return result;
}

ExcelValue tables_c14() {
  static ExcelValue result;
  if(variable_set[92] == 1) { return result;}
  result = _common5();
  variable_set[92] = 1;
  return result;
}

// end Tables

// start (innapropriate) sheet name!
ExcelValue s_innapropriate_sheet_name__c4() {
  static ExcelValue result;
  if(variable_set[93] == 1) { return result;}
  result = valuetypes_a3();
  variable_set[93] = 1;
  return result;
}

// end (innapropriate) sheet name!

// Start of named references
// End of named references
