#include <stdint.h>
#include <cctype>
#include <string_view>
#include <fstream>
#include <stdlib.h>
#include <filesystem>
#include <stdio.h>

typedef uint64_t u64;
typedef  int64_t i64;
typedef uint32_t u32;
typedef  int32_t i32;

struct Input {
	char* base;
	u64   len;
	char* cursor;
	u32 line;
	u32 col;
};

char peek(Input* input, i32 offset = 0) {
	return input->cursor[offset]; //todo bounds check
}

void eat_character(Input* input)
{
	if(*input->cursor == '\n') {
		input->col = 1;
		input->line++;
	}
	else {
		input->col++;
	}

	input->cursor++;
}


void eat_whitespace(Input* input)
{
	while((input->cursor - input->base) < input->len && std::isspace(*input->cursor)) {
		eat_character(input);
	}
}

void eat_until(Input* input, char stop_at)
{
	while((input->cursor - input->base) < input->len && *input->cursor != stop_at) {
		eat_character(input);
	}
}

void eat_ident(Input* input)
{
	char c;
	while((input->cursor - input->base) < input->len && (('0' <= (c = *input->cursor) && c <= '9') || ('A' <= c && c <= 'z') || ('a' <= c && c <= 'z') || c == '_')) {
		eat_character(input);
	}
}

struct Token {
	enum Type {
		_EOF,
		TYPE,
		IDENT,
		LBRACE,
		RBRACE,
		SEMICOLON,
		COMMENT,
		PREPROC_DIRECTIVE,
	} type;
	u32 line;
	u32 col;
	std::string_view value;
};

static char const* TokenTypeStrings[] = {
	"_EOF",
	"TYPE",
	"IDENT",
	"LBRACE",
	"RBRACE",
	"SEMICOLON",
	"COMMENT",
	"PREPROC_DIRECTIVE",
};

#define TYPE_NAME_LEN 12
static char const* TYPE_NAMES[TYPE_NAME_LEN] = {
	"u32",
	"i32",
	"f32",

	"struct",
	"enum",

	"u64",
	"i64",
	"f64",
	
	"u8",
	"i8",
	"u16",
	"i16",
};


Token get_next_token(Input* input)
{
	eat_whitespace(input);
	Token token {
		.line = input->line,
		.col  = input->col,
	};

	if(input->cursor - input->base >= input->len) {
		token.type = Token::Type::_EOF;
		return token;
	}

	if(peek(input) == '/' && peek(input, 1) == '/') {
		token.type = Token::Type::COMMENT;
		char* start = input->cursor + 2;
		eat_until(input, '\n');
		if(peek(input, -1) == '\r') input->cursor--;
		token.value = { start, (size_t)(input->cursor - start) };
	}
	else if(peek(input) == '/' && peek(input, 1) == '*') {
		token.type = Token::Type::COMMENT;
		char* start = input->cursor += 2;
		do {
			eat_until(input, '*');
			input->cursor++;
		} while(peek(input) != '/');
		token.value = { start, (size_t)(input->cursor - start - 1) };
		input->cursor++;
	}
	else if(peek(input) == '#') {
		token.type = Token::Type::PREPROC_DIRECTIVE;
		char* start = input->cursor + 1;
		eat_until(input, '\n');
		if(peek(input, -1) == '\r') input->cursor--;
		token.value = { start, (size_t)(input->cursor - start) };
	}
	else {
		switch(peek(input)) {
			case '{':
				token.type = Token::Type::LBRACE;
				token.value = { input->cursor, 1 };
				eat_character(input);
				break;
			case '}':
				token.type = Token::Type::RBRACE;
				token.value = { input->cursor, 1 };
				eat_character(input);
				break;
			case ';':
				token.type = Token::Type::SEMICOLON;
				token.value = { input->cursor, 1 };
				eat_character(input);
				break;

			default: {
				char* start = input->cursor;
				eat_ident(input);
				token.value = { start, (size_t)(input->cursor - start) };

				if(peek(input, -1) == '*') {
					token.type = Token::Type::TYPE;
				}
				else {
					token.type = Token::Type::IDENT;
					for(u32 i = 0; i < TYPE_NAME_LEN; i++) {
						if(token.value == TYPE_NAMES[i]) {
							token.type = Token::Type::TYPE;
							break;
						}
					}
				}
			}
		}
	}

	return token;
}

#define VALUE(token) (u32)(token).value.size(), (token).value.data()

int main()
{
	char const* path_input  = "input.h";
	char const* path_output_serializer = "serializer.h";
	char const* path_output_intermediate = "intermediate.cef";

	u64 input_size = std::filesystem::file_size(path_input);
	printf("input is %lld bytes\n", input_size);
	char* input_storage = (char*)malloc(input_size);
	Input input {
		.base = input_storage,
		.len  = input_size,
		.cursor = input_storage,
		.line = 1,
		.col  = 1,
	};
	FILE* input_stream = fopen(path_input, "rb");
	fread(input.base, input.len, 1, input_stream);
	fclose(input_stream);


	Token token;
	do {
		token = get_next_token(&input);
		printf("%10s: [L%d:%d] '%.*s'\n", TokenTypeStrings[token.type], token.line, token.col, VALUE(token));
	} while(token.type != Token::Type::_EOF);



	//
	// emmitting json serializer
	//

	enum {
		FUNC,
		MEMBER,
	} writer_stage = FUNC;

	char const* FORMAT_SPECS[] {
		 "hhu", //u8 
		 "hhd", //i8 
		 "hu",  //u16
		 "hd",  //i16
		 "u",   //u32
		 "d",   //i32
		 "llu", //u64
		 "lld", //i64
		 "f",   //f32
		 "llf", //f64
	};
	char const* next_format_spec = "d";
	u32 member_indent = 0;
	i32 brace_balance = 0;

	bool next_token_is_struct_name = false;
	bool searching_for_struct_name = false;
	i32  brace_balance_at_struct_name = 0;
	bool if_next_token_is_indent_ignore_it = false;
	bool last_token_had_ignore_check = false;
	bool next_member_has_comma_before_it = false;
	struct Buffer {
		char* data;
		u32   capacity;
		u32   used;
	};
	Buffer nested_member_path {
		.data     = (char*)malloc(1024),
		.capacity = 1024,
		.used     = 0,
	};

	struct {
		char* input_cursor;
		u32 member_indent;
		i32 brace_balance;
	} rewind_storage;

	FILE* output = fopen(path_output_serializer, "w");

	fprintf(output, R""(#include "%s"
#include <stdio.h>

)"", path_input);

	input.cursor = input.base;
	do {
		token = get_next_token(&input);
		switch(token.type) {
			case Token::Type::TYPE: {
				if(token.value == "struct") {
					next_token_is_struct_name = true;
					rewind_storage = {
						input.cursor,
						member_indent,
						brace_balance,
					};
					brace_balance_at_struct_name = brace_balance;
				}
				else {
					//slightly cursed but what are you going to do ¯\_(ツ)_/¯
					     if(token.value == "u8" ) next_format_spec = FORMAT_SPECS[0];
					else if(token.value == "i8" ) next_format_spec = FORMAT_SPECS[1];
					else if(token.value == "u16") next_format_spec = FORMAT_SPECS[2];
					else if(token.value == "i16") next_format_spec = FORMAT_SPECS[3];
					else if(token.value == "u32") next_format_spec = FORMAT_SPECS[4];
					else if(token.value == "i32") next_format_spec = FORMAT_SPECS[5];
					else if(token.value == "u64") next_format_spec = FORMAT_SPECS[6];
					else if(token.value == "i64") next_format_spec = FORMAT_SPECS[7];
					else if(token.value == "f32") next_format_spec = FORMAT_SPECS[8];
					else if(token.value == "f64") next_format_spec = FORMAT_SPECS[9];
				}
			} break;

			case Token::Type::IDENT: {
				if(if_next_token_is_indent_ignore_it) {
					if_next_token_is_indent_ignore_it = false;
					break;
				}


				if(searching_for_struct_name) {
					if(next_token_is_struct_name) {
						searching_for_struct_name = false;
						input.cursor  = rewind_storage.input_cursor;
						member_indent = rewind_storage.member_indent;
						brace_balance = rewind_storage.brace_balance;
					}
					else {
						break;
					}
				}


				switch(writer_stage) {
					case FUNC:
						// use a reference (&) here sp we can just always use the dot operator to get the next field. thanks c.
						fprintf(output, R""(char* json_serialize(%.*s const& packet, char* destination) {
	destination += sprintf(destination, R"-({
	"Type": "%.*s")-");
)"", VALUE(token), VALUE(token));
						writer_stage = MEMBER;
						next_member_has_comma_before_it = true;
						break;

					case MEMBER:
						if(next_member_has_comma_before_it) {
							fprintf(output, R""(	destination += sprintf(destination, ",");
)"");
						}

						if(next_token_is_struct_name) {
							//could use member indent here
							fprintf(output, R""(	destination += sprintf(destination, R"-(
	"%.*s": )-");
)"", VALUE(token));
							next_member_has_comma_before_it = false;
						}
						else {
							//could use member indent here
							fprintf(output, R""(	destination += sprintf(destination, R"-(
	"%.*s": %%%s)-", packet%.*s.%.*s);
)"", VALUE(token), next_format_spec, nested_member_path.used, nested_member_path.data, VALUE(token));
							next_member_has_comma_before_it = true;
						}

						if(next_token_is_struct_name) {
							u32 written = snprintf(nested_member_path.data + nested_member_path.used, nested_member_path.capacity, ".%.*s", VALUE(token));
							nested_member_path.used += written;
						}
						break;
				}

				next_token_is_struct_name = false;
			} break;

			case Token::Type::LBRACE: {
				// aahh c. `struct { u32 x } my_instance;` is valid and useful
				// might need to make this whole thing recursive to support multiple levels
				if(next_token_is_struct_name) {
					next_token_is_struct_name = false;
					searching_for_struct_name = true;
				}
				brace_balance++;

				if(member_indent > 0 && !searching_for_struct_name) fprintf(output, R""(	destination += sprintf(destination, "{");
)"");

				member_indent++;
			} break;

			case Token::Type::RBRACE: {
				brace_balance--;
				if(searching_for_struct_name && brace_balance_at_struct_name == brace_balance) {
					next_token_is_struct_name = true;
				}
				else {
					if_next_token_is_indent_ignore_it = true;
				}

				member_indent--;
				while(nested_member_path.used > 0) {
					if(*(nested_member_path.data + nested_member_path.used--) == '.') break;
				}

				if(!searching_for_struct_name)
					fprintf(output, R""(	destination += sprintf(destination, R"(
	})");
)"");

				if(member_indent == 0) {
					fprintf(output, R""(	return destination;
}

)"");
					writer_stage = FUNC;
				}
			} break;
		}
		if(token.type != Token::Type::COMMENT && last_token_had_ignore_check) if_next_token_is_indent_ignore_it = false;
		last_token_had_ignore_check = if_next_token_is_indent_ignore_it;
	} while(token.type != Token::Type::_EOF);
	fclose(output);
}
