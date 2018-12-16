#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <vector>

// keywords are reserved words that are counted as separate tokens; we make no distinction between keywords and operators; keyword prefix disambiguation
// is ensured when list is traversed sequentially in one direction -- keywords that are prefixes to other keywords come later in the list
//
// disambiguation example at front-to-back traversal:
//
//   this_is_a_keyword,
//   this // another keyword
//
const char* keywords[] = { // keep order in sync with Token enum below to maintain a mapping shortcut
	"(",
	")",
	"let",
	"+",
	"-",
	"*",
	"/"
};

// unlike keywords, a contiguous sequence of separators collapses into a single separator, which vanishes before reaching the token stream
const char separators[] = {
	' ',
	'\t',
	'\r',
	'\n' // keep new-line separator last
};

// tokens represent all known lexical categries -- keywords, literals and identifiers, plus the 'unknown' category
enum Token : uint16_t { // keep order in sync with keywords above to maintain a mapping shortcut
	TOKEN_UNKNOWN,
	TOKEN_PARENTHESIS_L,
	TOKEN_PARENTHESIS_R,
	TOKEN_LET,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_LITERAL_I32,
	TOKEN_LITERAL_F32,
	TOKEN_IDENTIFIER
};

const char* stringFromToken(const Token t)
{
	switch (t) {
	case TOKEN_UNKNOWN:
		return "TOKEN_UNKNOWN";
	case TOKEN_PARENTHESIS_L:
		return "TOKEN_PARENTHESIS_L";
	case TOKEN_PARENTHESIS_R:
		return "TOKEN_PARENTHESIS_R";
	case TOKEN_LET:
		return "TOKEN_LET";
	case TOKEN_PLUS:
		return "TOKEN_PLUS";
	case TOKEN_MINUS:
		return "TOKEN_MINUS";
	case TOKEN_MUL:
		return "TOKEN_MUL";
	case TOKEN_DIV:
		return "TOKEN_DIV";
	case TOKEN_LITERAL_I32:
		return "TOKEN_LITERAL_I32";
	case TOKEN_LITERAL_F32:
		return "TOKEN_LITERAL_F32";
	case TOKEN_IDENTIFIER:
		return "TOKEN_IDENTIFIER";
	}

	return "alien-token";
}

// a token as found in the source stream
struct TokenInStream {
	const char* loc;
	uint32_t len;
	uint32_t row;
	uint32_t col;
	union {
		int32_t literal_i32;
		float   literal_f32;
	};
	Token token;

	void print(FILE* f) const;
};

void TokenInStream::print(FILE* f) const
{
	const char* const stringToken = stringFromToken(token);

	if (TOKEN_LITERAL_I32 == token) {
		fprintf(f, "{\n\t"
			"%s: %d\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, literal_i32, loc, len, row, col);
	}
	else
	if (TOKEN_LITERAL_F32 == token) {
		fprintf(f, "{\n\t"
			"%s: %f\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, literal_f32, loc, len, row, col);
	}
	else
	if (TOKEN_IDENTIFIER == token) {
		fprintf(f, "{\n\t"
			"%s: %.*s\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, len, loc, loc, len, row, col);
	}
	else {
		fprintf(f, "{\n\t"
			"%s\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, loc, len, row, col);
	}
}

// countof helper
template < typename T, size_t N >
int8_t (& noneval_countof(const T (&)[N]))[N];

#define countof(x) sizeof(noneval_countof(x))

// check a stream location for stream terminator
bool isTerminator(const char* str)
{
	assert(str);
	return '\0' == *str;
}

enum TriState : uint8_t {
	TRI_FALSE,
	TRI_PRIME,
	TRI_SECOND
};

// check a stream location for separators; return TRI_FALSE if not a separator, TRI_PRIME if separator is new-line, and TRI_SECOND otherwise
TriState isSeparator(const char* str)
{
	assert(countof(separators));

	if (isTerminator(str))
		return TRI_SECOND;

	for (size_t i = 0; i < countof(separators) - 1; ++i)
		if (*str == separators[i])
			return TRI_SECOND;

	if (*str == separators[countof(separators) - 1])
		return TRI_PRIME;

	return TRI_FALSE;
}

// check if a stream location can be a part of an identifier; allowed: 0-9 A-Z _ a-z
bool isAllowedIdentifier(const char* str)
{
	if (isSeparator(str))
		return false;

	if ('0' <= *str && '9' >= *str)
		return true;

	if ('A' <= *str && 'Z' >= *str)
		return true;

	if ('_' == *str)
		return true;

	if ('a' <= *str && 'z' >= *str)
		return true;

	return false;
}

// check if a stream location can be a part of a numeric literal; allowed: 0-9 A-F a-f
bool isAllowedLiteral(const char* str)
{
	if (isSeparator(str))
		return false;

	if ('0' <= *str && '9' >= *str)
		return true;

	if ('A' <= *str && 'F' >= *str)
		return true;

	if ('a' <= *str && 'f' >= *str)
		return true;

	return false;
}

// check if a stream location is a positive/negative sign; return TRI_FALSE if not a sign, TRI_PRIME if positive, and TRI_SECOND if negative
TriState isSign(const char* str)
{
	assert(str);

	if ('+' == *str)
		return TRI_PRIME;

	if ('-' == *str)
		return TRI_SECOND;

	return TRI_FALSE;
}

// check if a stream location is a decimal point
bool isDecimalPoint(const char* str)
{
	assert(str);

	if ('.' == *str)
		return true;

	return false;
}

// get a single context-free token from the specified stream location; tokens are recognized as belonging to one of four categories, in decreasing precedence:
// literals > keywords > identifiers > unknown
Token getToken(const char* str, size_t& tokenLen, int32_t& lit_i32, float& lit_f32)
{
	assert(!isSeparator(str));

	// check for a numeric literal
	const char* tokend = str;
	const TriState hassign = isSign(tokend);
	bool hex = false;

	// a numeric literal may start with a sign, but any subsequent occurrences of signs void the literal
	if (hassign)
		tokend++;

	// a numeric literal may have a hexadecimal prefix
	if ('0' == tokend[0] && ('X' == tokend[1] || 'x' == tokend[1])) {
		tokend += 2;
		hex = true;
	}

	while (isAllowedLiteral(tokend))
		tokend++;

	// a numeric literal may contain a decimal point, but any subsequent occurrences of decimal points void the literal
	if (isDecimalPoint(tokend)) {
		tokend++;

		while (isAllowedLiteral(tokend))
			tokend++;
	}

	// heuristics to tell literals from literal-prefixed identifiers: if a literal ends with an identifier-allowed character, next character should not be identifier-allowed
	if (tokend != str && !isSign(tokend) && !isDecimalPoint(tokend) && (!isAllowedIdentifier(tokend - 1) || !isAllowedIdentifier(tokend))) {
		const size_t toklen = tokend - str;
		size_t consumed = 0;

		if (hex) {
			// try reading a hexadecimal integer
			int i32;
			const size_t offset = hassign ? 3 : 2; // account for sign and hex prefix

			// get the numeral by absolute value, then adjust the sign if needed
			if (1 == sscanf(str + offset, "%x%n", &i32, &consumed) && consumed == toklen - offset) {
				lit_i32 = TRI_SECOND == hassign ? -i32 : i32;
				tokenLen = toklen;
				return TOKEN_LITERAL_I32;
			}
		}
		else {
			// try reading a decimal integer
			int i32;
			if (1 == sscanf(str, "%d%n", &i32, &consumed) && consumed == toklen) {
				lit_i32 = i32;
				tokenLen = toklen;
				return TOKEN_LITERAL_I32;
			}
		}

		// try reading a float, either decimal or hexadecimal
		float f32;
		if (1 == sscanf(str, "%f%n", &f32, &consumed) && consumed == toklen) {
			lit_f32 = f32;
			tokenLen = toklen;
			return TOKEN_LITERAL_F32;
		}
	}

	// check for keywords; traverse keyword list front-to-back
	for (size_t i = 0; i < countof(keywords); ++i) {
		const size_t keywordLen = strlen(keywords[i]);
		assert(keywordLen);

		// heuristics to tell keywords from keyword-prefixed identifiers: if a keyword ends with an identifier-allowed character, next character should not be identifier-allowed
		if (0 == strncmp(str, keywords[i], keywordLen) && (!isAllowedIdentifier(str + keywordLen - 1) || !isAllowedIdentifier(str + keywordLen))) {
			tokenLen = keywordLen;
			return Token(i + 1); // mapping shortcut avoids a level of indirection
		}
	}

	// check for an identifier
	tokend = str;
	while (isAllowedIdentifier(tokend))
		tokend++;

	if (tokend != str) {
		tokenLen = tokend - str;
		return TOKEN_IDENTIFIER;
	}

	// something unexpected
	return TOKEN_UNKNOWN;
}

// tokenize a stream into tokens, keeping track of stream rows and columns
bool tokenize(
	const char* str,
	std::vector<TokenInStream>& tokens,
	size_t& row,
	size_t& col)
{
	while (!isTerminator(str)) {
		const TriState sep = isSeparator(str);
		if (sep) {
			// at new-line advance row count and reset column count
			if (TRI_PRIME == sep) {
				row++;
				col = size_t(-1);
			}
			col++;
			str++;
			continue;
		}

		size_t tokenLen = 0;
		int32_t lit_i32 = 0;
		float lit_f32 = 0;
		const Token token = getToken(str, tokenLen, lit_i32, lit_f32);

		if (TOKEN_UNKNOWN == token) {
			fprintf(stderr, "syntax error at row, col: %u, %u\n", row, col);
			return false;
		}

		if (TOKEN_LITERAL_F32 == token) {
			const TokenInStream tis = { .loc = str, .len = tokenLen, .row = row, .col = col, .literal_f32 = lit_f32, .token = token };
			tokens.push_back(tis);
		}
		else {
			const TokenInStream tis = { .loc = str, .len = tokenLen, .row = row, .col = col, .literal_i32 = lit_i32, .token = token };
			tokens.push_back(tis);
		}

		col += tokenLen;
		str += tokenLen;
	}

	return true;
}

// Abstract Syntax Tree (AST) per function definition;
// kinds of AST nodes
enum ASTNodeType : uint16_t {
	ASTNODE_NONE,
	ASTNODE_LET,         // expression that introduces named variables via a dedicated scope
	ASTNODE_INIT,        // statement that initializes a single named variable; appears at the beginning of 'let' expressions
	ASTNODE_EVAL_VAR,    // variable evaluation expression
	ASTNODE_EVAL_FUN,    // function evaluation expression
	ASTNODE_LITERAL_I32, // integral literal expression
	ASTNODE_LITERAL_F32  // floating-point literal expression
};

const char* stringFromNodeType(const ASTNodeType t)
{
	switch (t) {
	case ASTNODE_NONE:
		return "ASTNODE_NONE";
	case ASTNODE_LET:
		return "ASTNODE_LET";
	case ASTNODE_INIT:
		return "ASTNODE_INIT";
	case ASTNODE_EVAL_VAR:
		return "ASTNODE_EVAL_VAR";
	case ASTNODE_EVAL_FUN:
		return "ASTNODE_EVAL_FUN";
	case ASTNODE_LITERAL_I32:
		return "ASTNODE_LITERAL_I32";
	case ASTNODE_LITERAL_F32:
		return "ASTNODE_LITERAL_F32";
	}

	return "alien-astnode-type";
}

typedef size_t ASTNodeIndex; // index in ASTNodes
typedef std::vector<ASTNodeIndex> ASTNodeIndices;

const ASTNodeIndex nullidx = ASTNodeIndex(-1);

// AST node -- the equivalent of an expression or a statement in a (sub-) program
struct ASTNode {
	union {
		struct {
			const char* ptr; // name of variable or function
			uint32_t len;    // length of name
		} name;
		int32_t literal_i32; // value of integral literal
		float   literal_f32; // value of floating-point literal
	};
	ASTNodeType    type;     // type of node
	ASTNodeIndex   parent;   // parent index
	ASTNodeIndices args;     // per argument/sub-expression index; last sub-expression is the one returned

	void print(FILE* f, const std::vector<ASTNode>& tree, const size_t depth) const;
};

void ASTNode::print(FILE* f, const std::vector<ASTNode>& tree, const size_t depth) const
{
	for (size_t i = 0; i < depth; ++i)
		fprintf(f, "  ");

	const char* const stringType = stringFromNodeType(type);

	switch (type) {
	case ASTNODE_INIT:
	case ASTNODE_EVAL_VAR:
	case ASTNODE_EVAL_FUN:
		fprintf(f, "%s: %.*s\n", stringType, name.len, name.ptr);
		break;
	case ASTNODE_LITERAL_I32:
		fprintf(f, "%s: %d\n", stringType, literal_i32);
		break;
	case ASTNODE_LITERAL_F32:
		fprintf(f, "%s: %f\n", stringType, literal_f32);
		break;
	default:
		fprintf(f, "%s\n", stringType);
		break;
	}

	for (ASTNodeIndices::const_iterator it = args.begin(); it != args.end(); ++it)
		tree[*it].print(f, tree, depth + 1);
}

typedef std::vector<ASTNode> ASTNodes;

// get the leading sub-span of matching left and right parenthesis in a stream span
size_t getMatchingParentheses(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len)
{
	assert(len);
	assert(start + len <= tokens.size());
	assert(TOKEN_PARENTHESIS_L == tokens[start].token);

	unsigned depth = 0;
	const std::vector<TokenInStream>::const_iterator begin = tokens.begin() + start + 1;
	const std::vector<TokenInStream>::const_iterator end = tokens.begin() + start + len;
	for (std::vector<TokenInStream>::const_iterator it = begin; it != end; ++it) {
		if (TOKEN_PARENTHESIS_R == it->token) {
			if (!depth)
				return it - tokens.begin() - start + 1; // account for right parenthesis
			depth--;
		}
		else
		if (TOKEN_PARENTHESIS_L == it->token)
			depth++;
	}

	return size_t(-1);
}

size_t getNode(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len,
	const ASTNodeIndex parent,
	ASTNodes& tree);

// get the leading 'let' AST node in a stream span; return number of tokens encompassed; -1 if error
size_t getNodeLet(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len,
	const ASTNodeIndex parent,
	ASTNodes& tree)
{
	assert(len);
	assert(start + len <= tokens.size());
	assert(TOKEN_PARENTHESIS_L == tokens[start].token);
	assert(nullidx != parent && parent < tree.size());

	const size_t span = getMatchingParentheses(tokens, start, len);

	// check for stray left parenthesis
	if (size_t(-1) == span) {
		fprintf(stderr, "invalid let at line %d, column %d\n",
			tokens[start].row,
			tokens[start].col);
		return size_t(-1);
	}

	size_t start_it = start + 1; // account for left parenthesis
	size_t span_it = span - 2; // account for both parentheses

	// introduce named variables and initialize those
	while (span_it) {
		// check basic prerequisites of 'init' expression: (x expr)
		if (4 > span_it || TOKEN_PARENTHESIS_L != tokens[start_it].token || TOKEN_IDENTIFIER != tokens[start_it + 1].token) {
			fprintf(stderr, "invalid var-init at line %d, column %d\n",
				tokens[start_it].row,
				tokens[start_it].col);
			return size_t(-1);
		}

		const size_t subspan = getMatchingParentheses(tokens, start_it, span_it);

		// check for missing right parenthesis
		if (size_t(-1) == subspan) {
			fprintf(stderr, "invalid var-init at line %d, column %d\n",
				tokens[start_it].row,
				tokens[start_it].col);
			return size_t(-1);
		}

		start_it += 1; // account for left parenthesis
		span_it -= subspan;
		size_t subspan_it = subspan - 2; // account for both parentheses

		ASTNode newnode;
		newnode.name.ptr = tokens[start_it].loc;
		newnode.name.len = tokens[start_it].len;
		newnode.type = ASTNODE_INIT;
		newnode.parent = parent;

		const ASTNodeIndex newnodeIdx = tree.size();
		tree.push_back(newnode);

		tree[parent].args.push_back(newnodeIdx);

		// account for identifier token itself
		start_it++;
		subspan_it--;

		const size_t initspan = getNode(tokens, start_it, subspan_it, newnodeIdx, tree);

		if (size_t(-1) == initspan) {
			fprintf(stderr, "invalid var-init at line %d, column %d\n",
				tokens[start_it].row,
				tokens[start_it].col);
			return size_t(-1);
		}

		start_it += initspan + 1; // account for right parenthesis
		assert(subspan_it == initspan);
	}

	return span;
}

// return the index of the 'init' statement of a named var; -1 if not found
ASTNodeIndex checkKnownVar(
	const char* name,
	const size_t len,
	const ASTNodeIndex parent,
	const ASTNodes& tree)
{
	if (nullidx == parent)
		return nullidx;

	assert(parent < tree.size());

	if (ASTNODE_LET == tree[parent].type) {
		for (ASTNodeIndices::const_iterator it = tree[parent].args.begin(); it != tree[parent].args.end(); ++it) {
			// only the leading subnodes of a 'let' expression are 'init' statements
			if (ASTNODE_INIT != tree[*it].type)
				break;

			if (tree[*it].name.len == len && 0 == strncmp(tree[*it].name.ptr, name, len))
				return *it;
		}
	}

	return checkKnownVar(name, len, tree[parent].parent, tree);
}

// get the leading AST node in a stream span; return number of tokens encompassed; -1 if error
size_t getNode(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len,
	const ASTNodeIndex parent,
	ASTNodes& tree)
{
	assert(nullidx == parent || parent < tree.size());

	// check for stray right parenthesis
	if (TOKEN_PARENTHESIS_R == tokens[start].token) {
		fprintf(stderr, "stray right parentesis at line %d, column %d\n",
			tokens[start].row,
			tokens[start].col);
		return size_t(-1);
	}

	ASTNode newnode;
	const ASTNodeIndex newnodeIdx = tree.size();

	// check for parenthesized expressions like function calls and scopes
	if (TOKEN_PARENTHESIS_L == tokens[start].token) {
		const size_t span = getMatchingParentheses(tokens, start, len);

		// check for stray left parenthesis
		if (size_t(-1) == span) {
			fprintf(stderr, "stray left parentesis at line %d, column %d\n",
				tokens[start].row,
				tokens[start].col);
			return size_t(-1);
		}

		// check for empty expression
		if (2 == span) {
			fprintf(stderr, "empty parenteses at line %d, column %d\n",
				tokens[start].row,
				tokens[start].col);
			return size_t(-1);
		}

		size_t start_it = start + 1; // account for left parenthesis
		size_t span_it = span - 2; // account for both parentheses

		switch (tokens[start_it].token) {
			size_t subspan;
		case TOKEN_LET:
			// check basic prerequisites of 'let' expression: let ((x expr))
			if (7 > span_it || TOKEN_PARENTHESIS_L != tokens[start_it + 1].token) {
				fprintf(stderr, "invalid let at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}

			newnode.name.ptr = nullptr;
			newnode.name.len = 0;
			newnode.type = ASTNODE_LET;
			newnode.parent = parent;

			tree.push_back(newnode);

			if (nullidx != parent)
				tree[parent].args.push_back(newnodeIdx);

			// account for keyword token itself
			start_it++;
			span_it--;

			// introduce named vars and their dedicated scope
			subspan = getNodeLet(tokens, start_it, span_it, newnodeIdx, tree);

			if (size_t(-1) == subspan)
				return size_t(-1);

			start_it += subspan;
			span_it -= subspan;
			break;

		// check for various function calls
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_MUL:
		case TOKEN_DIV:
		case TOKEN_IDENTIFIER: // TODO: check against known function identifiers
			newnode.name.ptr = tokens[start_it].loc;
			newnode.name.len = tokens[start_it].len;
			newnode.type = ASTNODE_EVAL_FUN;
			newnode.parent = parent;

			tree.push_back(newnode);

			if (nullidx != parent)
				tree[parent].args.push_back(newnodeIdx);

			// account for keyword/identifier token itself
			start_it++;
			span_it--;
			break;

		default:
			return size_t(-1);
		}

		// check for an expression sequence
		while (span_it) {
			const size_t subspan = getNode(tokens, start_it, span_it, newnodeIdx, tree);

			if (size_t(-1) == subspan)
				return size_t(-1);

			start_it += subspan;
			span_it -= subspan;
		}

		return span;
	}

	// check for various single-token nodes
	switch (tokens[start].token) {
		ASTNodeIndex initIdx;
	case TOKEN_LITERAL_I32:
		newnode.literal_i32 = tokens[start].literal_i32;
		newnode.type = ASTNODE_LITERAL_I32;
		newnode.parent = parent;
		break;

	case TOKEN_LITERAL_F32:
		newnode.literal_f32 = tokens[start].literal_f32;
		newnode.type = ASTNODE_LITERAL_F32;
		newnode.parent = parent;
		break;

	case TOKEN_IDENTIFIER:
		// check against known var identifiers
		initIdx = checkKnownVar(tokens[start].loc, tokens[start].len, parent, tree);

		if (nullidx == initIdx) {
			fprintf(stderr, "unknown var at line %d, column %d\n",
				tokens[start].row,
				tokens[start].col);
			return size_t(-1);
		}

		newnode.name.ptr = tokens[start].loc;
		newnode.name.len = tokens[start].len;
		newnode.type = ASTNODE_EVAL_VAR;
		newnode.parent = parent;
		break;

	default:
		return size_t(-1);
	}

	tree.push_back(newnode);

	if (nullidx != parent)
		tree[parent].args.push_back(newnodeIdx);

	return 1;
}

// unittest
int main(int argc, char** argv)
{
	FILE* infile = stdin;

	if (argc == 2) {
		infile = fopen(argv[1], "r");
		if (nullptr == infile) {
			fprintf(stdout, "failure reading input file\n");
			return -1;
		}
	}

	const size_t bufsize = 1 << 20;
	std::vector<char> buffer(bufsize);

	const size_t readCount = fread(buffer.data(), sizeof(buffer.front()), buffer.size() - 1, infile);

	if (infile != stdin)
		fclose(infile);

	if (0 == readCount)
		return 0;

	std::vector<TokenInStream> tokens;
	tokens.reserve(1024);

	size_t row = 0;
	size_t col = 0;

	if (!tokenize(buffer.data(), tokens, row, col)) {
		fprintf(stdout, "failure\n");
		return -1;
	}

#if 0
	for (std::vector<TokenInStream>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
		it->print(stdout);
		putc('\n', stdout);
	}

#endif
	ASTNodes tree;
	const ASTNode root = { .name = { .ptr = nullptr, .len = 0 }, .type = ASTNODE_NONE, .parent = nullidx, .args = ASTNodeIndices() };
	tree.push_back(root);

	size_t start_it = 0;
	size_t len_it = tokens.size();
	while (len_it) {
		const size_t span = getNode(tokens, start_it, len_it, 0, tree);

		if (size_t(-1) == span) {
			fprintf(stdout, "failure\n");
			return -1;
		}

		start_it += span;
		len_it -= span;
	}

#if 0 // no use of printing the dummy root node
	tree.front().print(stdout, tree, 0);

#else
	for (ASTNodeIndices::const_iterator it = tree.front().args.begin(); it != tree.front().args.end(); ++it)
		tree[*it].print(stdout, tree, 0);

#endif
	fprintf(stdout, "success\n");
	return 0;
}
