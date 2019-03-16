#include <stdint.h>
#include <stdlib.h>
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
	"defun",
	"let",
	"+",
	"-",
	"*",
	"/",
	"ifzero",
	"ifneg",
	"print",
	"readi32",
	"readf32"
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
	// keywords
	TOKEN_PARENTHESIS_L,
	TOKEN_PARENTHESIS_R,
	TOKEN_DEFUN,
	TOKEN_LET,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_IFZERO,
	TOKEN_IFNEG,
	TOKEN_PRINT,
	TOKEN_READ_I32,
	TOKEN_READ_F32,
	// literals
	TOKEN_LITERAL_I32,
	TOKEN_LITERAL_F32,
	// identifiers
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
	case TOKEN_DEFUN:
		return "TOKEN_DEFUN";
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
	case TOKEN_IFZERO:
		return "TOKEN_IFZERO";
	case TOKEN_IFNEG:
		return "TOKEN_IFNEG";
	case TOKEN_PRINT:
		return "TOKEN_PRINT";
	case TOKEN_READ_I32:
		return "TOKEN_READ_I32";
	case TOKEN_READ_F32:
		return "TOKEN_READ_F32";
	case TOKEN_LITERAL_I32:
		return "TOKEN_LITERAL_I32";
	case TOKEN_LITERAL_F32:
		return "TOKEN_LITERAL_F32";
	case TOKEN_IDENTIFIER:
		return "TOKEN_IDENTIFIER";
	}

	return "alien-token";
}

// a ref to an immutable sequence of T
template < typename T >
struct SeqRef {
	const T* ptr; // sequence without sentinel
	uint32_t len; // length of sequence
};

typedef SeqRef< char > StrRef;

// a token as found in the source stream
struct TokenInStream {
	StrRef   val;
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
			stringToken, literal_i32, val.ptr, val.len, row, col);
	}
	else
	if (TOKEN_LITERAL_F32 == token) {
		fprintf(f, "{\n\t"
			"%s: %f\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, literal_f32, val.ptr, val.len, row, col);
	}
	else
	if (TOKEN_IDENTIFIER == token) {
		fprintf(f, "{\n\t"
			"%s: %.*s\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, val.len, val.ptr, val.ptr, val.len, row, col);
	}
	else {
		fprintf(f, "{\n\t"
			"%s\n\t"
			"%p, %u\n\t"
			"%u, %u\n}",
			stringToken, val.ptr, val.len, row, col);
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
Token getToken(const char* str, uint32_t& tokenLen, int32_t& lit_i32, float& lit_f32)
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
		int consumed = 0;

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
	std::vector<TokenInStream>& tokens)
{
	uint32_t row = 0;
	uint32_t col = 0;

	while (!isTerminator(str)) {
		const TriState sep = isSeparator(str);
		if (sep) {
			// at new-line advance row count and reset column count
			if (TRI_PRIME == sep) {
				row++;
				col = uint32_t(-1);
			}
			col++;
			str++;
			continue;
		}

		uint32_t tokenLen = 0;
		int32_t lit_i32 = 0;
		float lit_f32 = 0;
		const Token token = getToken(str, tokenLen, lit_i32, lit_f32);

		if (TOKEN_UNKNOWN == token) {
			fprintf(stderr, "syntax error at row, col: %u, %u\n", row, col);
			return false;
		}

		if (TOKEN_LITERAL_F32 == token) {
			const TokenInStream tis = { .val = { .ptr = str, .len = tokenLen }, .row = row, .col = col, .literal_f32 = lit_f32, .token = token };
			tokens.push_back(tis);
		}
		else {
			const TokenInStream tis = { .val = { .ptr = str, .len = tokenLen }, .row = row, .col = col, .literal_i32 = lit_i32, .token = token };
			tokens.push_back(tis);
		}

		col += tokenLen;
		str += tokenLen;
	}

	return true;
}

// Abstract Syntax Tree (AST)
// AST node semantical types
enum ASTNodeType : uint16_t {
	ASTNODE_LET,         // expression that introduces named variables via a nested scope
	ASTNODE_INIT,        // statement that initializes a single named variable; appears at the beginning of 'let' expressions
	ASTNODE_EVAL_VAR,    // variable evaluation expression
	ASTNODE_EVAL_FUN,    // function evaluation expression
	ASTNODE_LITERAL_I32, // integral literal expression
	ASTNODE_LITERAL_F32  // floating-point literal expression
};
// note: DEFUN does not have a dedicted node type; instead we re-purpose a LET node into a statement that is a
// nop for linear execution, but introduces a LET scope of initialized-from-args named vars when branched to; we
// differentiate LET expressions from DEFUN statements by the fact the latter are named while the former are not

const char* stringFromNodeType(const ASTNodeType t)
{
	switch (t) {
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
		StrRef  name;        // name of variable or function, AKA identifier
		int32_t literal_i32; // value of integral literal
		float   literal_f32; // value of floating-point literal
	};
	ASTNodeType    type;     // semantical type of node
	ASTNodeIndex   parent;   // parent index
	ASTNodeIndex   eval;     // eval semantics target
	ASTNodeIndices args;     // per argument/sub-expression index

	void print(FILE* f, const std::vector<ASTNode>& tree, const size_t depth) const;
	bool isDefun() const { return ASTNODE_LET == type && name.ptr; }
};

// special eval targets for built-in functions, AKA intrinsics
enum : ASTNodeIndex {
	INTRIN_PLUS     = ASTNodeIndex(-2),
	INTRIN_MINUS    = ASTNodeIndex(-3),
	INTRIN_MUL      = ASTNodeIndex(-4),
	INTRIN_DIV      = ASTNodeIndex(-5),
	INTRIN_IFZERO   = ASTNodeIndex(-6),
	INTRIN_IFNEG    = ASTNodeIndex(-7),
	INTRIN_PRINT    = ASTNodeIndex(-8),
	INTRIN_READ_I32 = ASTNodeIndex(-9),
	INTRIN_READ_F32 = ASTNodeIndex(-10)
};

ASTNodeIndex getEvalTarget(const Token token)
{
	switch (token) {
	case TOKEN_PLUS:
		return INTRIN_PLUS;
	case TOKEN_MINUS:
		return INTRIN_MINUS;
	case TOKEN_MUL:
		return INTRIN_MUL;
	case TOKEN_DIV:
		return INTRIN_DIV;
	case TOKEN_IFZERO:
		return INTRIN_IFZERO;
	case TOKEN_IFNEG:
		return INTRIN_IFNEG;
	case TOKEN_PRINT:
		return INTRIN_PRINT;
	case TOKEN_READ_I32:
		return INTRIN_READ_I32;
	case TOKEN_READ_F32:
		return INTRIN_READ_F32;
	}

	return nullidx;
}

void ASTNode::print(FILE* f, const std::vector<ASTNode>& tree, const size_t depth) const
{
	for (size_t i = 0; i < depth; ++i)
		fprintf(f, "  ");

	const char* const stringType = stringFromNodeType(type);

	switch (type) {
	case ASTNODE_LET:
		assert(name.ptr && name.len || !name.ptr && !name.len);
		if (name.ptr)
			fprintf(f, "%s: %.*s\n", stringType, name.len, name.ptr);
		else
			fprintf(f, "%s\n", stringType);
		break;
	case ASTNODE_INIT:
	case ASTNODE_EVAL_VAR:
	case ASTNODE_EVAL_FUN:
		assert(name.ptr && name.len);
		fprintf(f, "%s: %.*s\n", stringType, name.len, name.ptr);
		break;
	case ASTNODE_LITERAL_I32:
		fprintf(f, "%s: %d\n", stringType, literal_i32);
		break;
	case ASTNODE_LITERAL_F32:
		fprintf(f, "%s: %f\n", stringType, literal_f32);
		break;
	default:
		assert(false);
		break;
	}

	for (ASTNodeIndices::const_iterator it = args.begin(); it != args.end(); ++it)
		tree[*it].print(f, tree, depth + 1);
}

typedef std::vector<ASTNode> ASTNodes;

// get the leading sub-span of matching left and right parenthesis in a token-stream span
size_t getMatchingParentheses(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len)
{
	assert(1 < len);
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

// get the number of sub-expressions or init statements to a given AST node; return init count if countInit is true, sub-expression count otherwise
size_t getSubCount(
	const bool countInit,
	const ASTNodeIndex parent,
	const ASTNodes& tree)
{
	assert(nullidx != parent && parent < tree.size());
	const ASTNode& node = tree[parent];
	assert(!countInit || ASTNODE_LET == node.type);

	ASTNodeIndices::const_iterator it = node.args.begin();

	// count leading 'init' statements
	for (; it != node.args.end(); ++it)
		if (ASTNODE_INIT != tree[*it].type)
			break;

	if (countInit)
		return it - node.args.begin();

	// count trailing sub-expressions
	size_t ret = 0;
	for (; it != node.args.end(); ++it) {
		// ignore 'defun' statements, i.e. named 'let' sub-nodes
		if (tree[*it].isDefun())
			continue;

		ret++;
	}

	return ret;
}

size_t getNode(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len,
	const ASTNodeIndex parent,
	ASTNodes& tree);

// get the leading 'let' AST node in a token-stream span; return number of tokens encompassed; -1 if error
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

		ASTNode newnode = { .name = tokens[start_it].val, .type = ASTNODE_INIT, .parent = parent };

		const ASTNodeIndex newnodeIdx = tree.size();
		tree.push_back(newnode);

		tree[parent].args.push_back(newnodeIdx);

		// account for identifier token itself
		start_it++;
		subspan_it--;

		const size_t initspan = getNode(tokens, start_it, subspan_it, newnodeIdx, tree);

		if (subspan_it != initspan) {
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

// get the leading 'defun' AST node in a token-stream span; return number of tokens encompassed; -1 if error
size_t getNodeDefun(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len,
	const ASTNodeIndex parent,
	ASTNodes& tree)
{
	assert(1 < len);
	assert(start + len <= tokens.size());
	assert(TOKEN_IDENTIFIER == tokens[start].token);

	if (TOKEN_PARENTHESIS_L != tokens[start + 1].token) {
		fprintf(stderr, "invalid defun at line %d, column %d\n",
			tokens[start].row,
			tokens[start].col);
		return size_t(-1);
	}

	size_t start_it = start + 1; // account for identifier
	size_t span_it = len - 1;

	const size_t span = getMatchingParentheses(tokens, start_it, span_it);

	// check for stray left parenthesis
	if (size_t(-1) == span) {
		fprintf(stderr, "invalid defun at line %d, column %d\n",
			tokens[start_it].row,
			tokens[start_it].col);
		return size_t(-1);
	}

	start_it++; // account for left parenthesis
	span_it = span - 2; // account for both parentheses

	// introduce named variables but don't initialize those
	while (span_it) {
		// check basic prerequisites of 'defun' version of 'init' expression: x
		if (TOKEN_IDENTIFIER != tokens[start_it].token) {
			fprintf(stderr, "invalid defun-arg at line %d, column %d\n",
				tokens[start_it].row,
				tokens[start_it].col);
			return size_t(-1);
		}

		ASTNode newnode = { .name = tokens[start_it].val, .type = ASTNODE_INIT, .parent = parent };

		const ASTNodeIndex newnodeIdx = tree.size();
		tree.push_back(newnode);
		tree[parent].args.push_back(newnodeIdx);

		// account for identifier token itself
		start_it++;
		span_it--;
	}

	return span + 1; // account for identifier
}

// return the index of the 'init' statement of a named var; -1 if not found
ASTNodeIndex checkKnownVar(
	const StrRef& name,
	const ASTNodeIndex parent,
	const ASTNodes& tree)
{
	assert(name.ptr);
	assert(name.len);

	if (nullidx == parent)
		return nullidx;

	assert(parent < tree.size());

	if (ASTNODE_LET == tree[parent].type) {
		for (ASTNodeIndices::const_iterator it = tree[parent].args.begin(); it != tree[parent].args.end(); ++it) {
			// only the leading subnodes of a 'let' expression are 'init' statements
			if (ASTNODE_INIT != tree[*it].type)
				break;

			if (name.len == tree[*it].name.len && 0 == strncmp(tree[*it].name.ptr, name.ptr, name.len))
				return *it;
		}
	}

	return checkKnownVar(name, tree[parent].parent, tree);
}

ASTNodeIndex checkKnownDefun(
	const StrRef& name,
	const ASTNodeIndex parent,
	const ASTNodes& tree)
{
	assert(name.ptr);
	assert(name.len);

	if (nullidx == parent)
		return nullidx;

	assert(parent < tree.size());

	// check all parent and grand-parent let-expressions and defun-statements
	if (ASTNODE_LET == tree[parent].type) {
		if (name.len == tree[parent].name.len && 0 == strncmp(tree[parent].name.ptr, name.ptr, name.len))
			return parent;

		// check all defun-sub-nodes of this (grand) parent
		for (ASTNodeIndices::const_iterator it = tree[parent].args.begin(); it != tree[parent].args.end(); ++it) {
			if (ASTNODE_LET != tree[*it].type)
				continue;

			if (name.len == tree[*it].name.len && 0 == strncmp(tree[*it].name.ptr, name.ptr, name.len))
				return *it;
		}
	}

	return checkKnownDefun(name, tree[parent].parent, tree);
}

const ssize_t max_ssize = size_t(-1) >> 1;

// return count of expected args for an AST node that is a function call; if count of args can vary, return the negated minimal count
// if no such known function, return max ssize_t; if a function is found, patch eval target at call site
ssize_t getMinFunArgs(
	const ASTNodeIndex parent,
	ASTNodes& tree)
{
	assert(nullidx != parent && parent < tree.size());
	ASTNode& node = tree[parent];
	assert(ASTNODE_EVAL_FUN == node.type);

	// check built-in functions
	switch (node.eval) {
	// arithmetic functions have a minimum number of args
	case INTRIN_PLUS:
	case INTRIN_MINUS:
	case INTRIN_MUL:
	case INTRIN_DIV:
		return -2;
	// all other functions have an exact number of args
	case INTRIN_IFZERO:
	case INTRIN_IFNEG:
		return 3;
	case INTRIN_PRINT:
		return 1;
	case INTRIN_READ_I32:
	case INTRIN_READ_F32:
		return 0;
	}

	// check the upper tree for a matching defun; at a match an exact number of args is returned
	const ASTNodeIndex defunIdx = checkKnownDefun(node.name, node.parent, tree);

	if (nullidx == defunIdx)
		return max_ssize;

	// patch the eval target of the invocation
	node.eval = defunIdx;
	return getSubCount(true, defunIdx, tree);
}

// get the leading AST node in a token-stream span; return number of tokens encompassed; -1 if error
size_t getNode(
	const std::vector<TokenInStream>& tokens,
	const size_t start,
	const size_t len,
	const ASTNodeIndex parent,
	ASTNodes& tree)
{
	assert(len);
	assert(start + len <= tokens.size());
	assert(nullidx != parent && parent < tree.size());

	// check for stray right parenthesis
	if (TOKEN_PARENTHESIS_R == tokens[start].token) {
		fprintf(stderr, "stray right parentesis at line %d, column %d\n",
			tokens[start].row,
			tokens[start].col);
		return size_t(-1);
	}

	ASTNode newnode = { .parent = parent };
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
		case TOKEN_DEFUN:
			// 'defun' statements are disallowed anywhere but in 'let' expressions for better lisp-ness
			if (ASTNODE_LET != tree[parent].type) {
				fprintf(stderr, "misplaced defun at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}

			// check basic prerequisites of 'defun' statement: defun f() expr
			if (5 > span_it || TOKEN_IDENTIFIER != tokens[start_it + 1].token) {
				fprintf(stderr, "invalid defun at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}

			// account for keyword token itself
			start_it++;
			span_it--;

			// node introduces a named scope
			newnode.name = tokens[start_it].val;
			newnode.type = ASTNODE_LET;

			tree.push_back(newnode);
			tree[parent].args.push_back(newnodeIdx);

			// introduce named args into their dedicated scope
			subspan = getNodeDefun(tokens, start_it, span_it, newnodeIdx, tree);

			if (size_t(-1) == subspan)
				return size_t(-1);

			start_it += subspan;
			span_it -= subspan;
			break;

		case TOKEN_LET:
			// check basic prerequisites of 'let' expression: let () expr
			if (4 > span_it || TOKEN_PARENTHESIS_L != tokens[start_it + 1].token) {
				fprintf(stderr, "invalid let at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}

			// node introduces an anonymous scope
			newnode.name.ptr = nullptr;
			newnode.name.len = 0;
			newnode.type = ASTNODE_LET;

			tree.push_back(newnode);
			tree[parent].args.push_back(newnodeIdx);

			// account for keyword token itself
			start_it++;
			span_it--;

			// introduce named vars into their dedicated scope
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
		case TOKEN_IFZERO:
		case TOKEN_IFNEG:
		case TOKEN_PRINT:
		case TOKEN_READ_I32:
		case TOKEN_READ_F32:
		case TOKEN_IDENTIFIER:
			newnode.name = tokens[start_it].val;
			newnode.type = ASTNODE_EVAL_FUN;
			newnode.eval = getEvalTarget(tokens[start_it].token);

			tree.push_back(newnode);
			tree[parent].args.push_back(newnodeIdx);

			// account for keyword/identifier token itself
			start_it++;
			span_it--;
			break;

		default:
			fprintf(stderr, "unexpected token at line %d, column %d\n",
				tokens[start].row,
				tokens[start].col);
			return size_t(-1);
		}

		// check for a sub-expression sequence
		while (span_it) {
			const size_t subspan = getNode(tokens, start_it, span_it, newnodeIdx, tree);

			if (size_t(-1) == subspan)
				return size_t(-1);

			start_it += subspan;
			span_it -= subspan;
		}

		// verify correct number of sub-expressions for the given expression
		switch (tree[newnodeIdx].type) {
			size_t subcount;
			ssize_t funargs;
		case ASTNODE_LET:
			// 'let' nodes, whether let-expressions or defun-statements, need at least one expression to return
			subcount = getSubCount(false, newnodeIdx, tree);
			if (0 == subcount) {
				fprintf(stderr, "invalid let/defun at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}
			break;
		case ASTNODE_EVAL_FUN:
			subcount = getSubCount(false, newnodeIdx, tree);
			funargs = getMinFunArgs(newnodeIdx, tree);

			// check if referenced function exists
			if (max_ssize == funargs) {
				fprintf(stderr, "unknown function call at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}

			// non-negative funargs means exact count, negative -- minimal count
			if (0 <= funargs ? subcount != funargs : subcount < -funargs) {
				fprintf(stderr, "invalid function call at line %d, column %d\n",
					tokens[start].row,
					tokens[start].col);
				return size_t(-1);
			}
			break;
		default:
			break;
		}

		return span;
	}

	// check for various single-token nodes
	switch (tokens[start].token) {
		ASTNodeIndex initIdx;
	case TOKEN_LITERAL_I32:
		newnode.literal_i32 = tokens[start].literal_i32;
		newnode.type = ASTNODE_LITERAL_I32;
		break;

	case TOKEN_LITERAL_F32:
		newnode.literal_f32 = tokens[start].literal_f32;
		newnode.type = ASTNODE_LITERAL_F32;
		break;

	case TOKEN_IDENTIFIER:
		// check against known var identifiers
		initIdx = checkKnownVar(tokens[start].val, parent, tree);

		if (nullidx == initIdx) {
			fprintf(stderr, "unknown var at line %d, column %d\n",
				tokens[start].row,
				tokens[start].col);
			return size_t(-1);
		}

		newnode.name = tokens[start].val;
		newnode.type = ASTNODE_EVAL_VAR;
		newnode.eval = initIdx;
		break;

	default:
		fprintf(stderr, "unexpected token at line %d, column %d\n",
			tokens[start].row,
			tokens[start].col);
		return size_t(-1);
	}

	tree.push_back(newnode);
	tree[parent].args.push_back(newnodeIdx);

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
// evaluation of (guaranteed correct) AST

enum Type {
	TYPE_I32,
	TYPE_F32
};

const char* stringFromType(const Type t)
{
	switch (t) {
	case TYPE_I32:
		return "TYPE_I32";
	case TYPE_F32:
		return "TYPE_F32";
	}

	return "alien-type";
}

struct Value {
	Type type;
	union {
		int32_t i32;
		float f32;
	};

	void print(FILE* f) const;
};

void Value::print(FILE* f) const
{
	fprintf(f, "%s", stringFromType(type));

	switch (type) {
	case TYPE_I32:
		fprintf(f, " %d\n", i32);
		break;
	case TYPE_F32:
		fprintf(f, " %f\n", f32);
		break;
	}
}

struct NamedValue {
	ASTNodeIndex name;
	Value        val;
};

typedef std::vector< NamedValue > VarStack;

Value eval(const ASTNodeIndex index, const ASTNodes& tree, VarStack& stack, int32_t nargs = 0);

template < int32_t BINOP_I32(int32_t, int32_t), float BINOP_F32(float, float) >
Value evalArith(const ASTNodeIndex index, const ASTNodes& tree, VarStack& stack)
{
	assert(nullidx != index && index < tree.size());
	const ASTNode& node = tree[index];

	int32_t acc_i32 = 0;
	float acc_f32 = 0;
	bool isF32 = false;

	// arithmetic intrinsics have at least two args
	ASTNodeIndices::const_iterator it = node.args.begin();
	const Value arg = eval(*it++, tree, stack);

	// promote computation to f32 at the first encounter of an f32 arg
	if (TYPE_F32 == arg.type) {
		acc_f32 = arg.f32;
		isF32 = true;
	}
	else {
		assert(TYPE_I32 == arg.type);
		acc_i32 = arg.i32;
	}

	if (!isF32) {
		for (; it != node.args.end(); ++it) {
			const Value arg = eval(*it, tree, stack);

			if (TYPE_F32 == arg.type) {
				++it; // we are done with this arg
				acc_f32 = BINOP_F32(float(acc_i32), arg.f32);
				isF32 = true;
				break;
			}

			assert(TYPE_I32 == arg.type);
			acc_i32 = BINOP_I32(acc_i32, arg.i32);
		}
	}

	if (isF32) {
		for (; it != node.args.end(); ++it) {
			const Value arg = eval(*it, tree, stack);

			if (TYPE_I32 == arg.type) {
				acc_f32 = BINOP_F32(acc_f32, float(arg.i32));
			}
			else {
				assert(TYPE_F32 == arg.type);
				acc_f32 = BINOP_F32(acc_f32, arg.f32);
			}
		}
	}

	return isF32
		? Value{ .type = TYPE_F32, { .f32 = acc_f32 } } // enclose anonymous union to workaround clang bug
		: Value{ .type = TYPE_I32, { .i32 = acc_i32 } };
}

template < typename T >
T binop_plus(T a, T b) { return a + b; }

template < typename T >
T binop_minus(T a, T b) { return a - b; }

template < typename T >
T binop_mul(T a, T b) { return a * b; }

template < typename T >
T binop_div(T a, T b) { return a / b; }

template < typename T >
T read(const char* prompt, const char* format)
{
	T ret;

	fprintf(stdout, prompt);

	if (1 != fscanf(stdin, format, &ret)) {
		fprintf(stderr, "runtime error: invalid input\n");
		exit(-1); // runtime input error
	}

	return ret;
}

Value eval(const ASTNodeIndex index, const ASTNodes& tree, VarStack& stack, int32_t nargs)
{
	assert(nullidx != index && index < tree.size());
	const ASTNode& node = tree[index];
	const uint32_t narg = 0 < nargs ? nargs : 0;
	const size_t stackRestore = stack.size() - narg;
	Value ret;

	switch (node.type) {
	case ASTNODE_LET:
		for (ASTNodeIndices::const_iterator it = node.args.begin(); it != node.args.end(); ++it) {
			if (tree[*it].isDefun())
				continue;

			ret = eval(*it, tree, stack, nargs--);
		}
		// pop args/locals from the var stack
		stack.resize(stackRestore);
		return ret;
	case ASTNODE_INIT:
		if (node.args.empty()) // is this a defun arg? it's already on the var stack -- just name it
			stack[stack.size() - narg].name = index;
		else // this is a local var -- init it and put it on the stack
			stack.push_back(NamedValue{ .name = index, .val = eval(node.args.front(), tree, stack) });
		return Value();
	case ASTNODE_EVAL_VAR:
		// scan the var stack from the top down for our var
		for (VarStack::const_reverse_iterator it = stack.rbegin(); it != stack.rend(); ++it)
			if (it->name == node.eval)
				return it->val;
		break;
	case ASTNODE_EVAL_FUN:
		switch (node.eval) {
		case INTRIN_PLUS:
			return evalArith< binop_plus< int32_t >, binop_plus< float > >(index, tree, stack);
		case INTRIN_MINUS:
			return evalArith< binop_minus< int32_t >, binop_minus< float > >(index, tree, stack);
		case INTRIN_MUL:
			return evalArith< binop_mul< int32_t >, binop_mul< float > >(index, tree, stack);
		case INTRIN_DIV:
			return evalArith< binop_div< int32_t >, binop_div< float > >(index, tree, stack);
		case INTRIN_IFZERO:
			assert(3 == node.args.size());
			ret = eval(node.args[0], tree, stack);

			if (TYPE_F32 == ret.type ? 0.f == ret.f32 : 0 == ret.i32)
				return eval(node.args[1], tree, stack);

			return eval(node.args[2], tree, stack);
		case INTRIN_IFNEG:
			assert(3 == node.args.size());
			ret = eval(node.args[0], tree, stack);

			if (TYPE_F32 == ret.type ? 0.f > ret.f32 : 0 > ret.i32)
				return eval(node.args[1], tree, stack);

			return eval(node.args[2], tree, stack);
		case INTRIN_PRINT:
			assert(1 == node.args.size());
			ret = eval(node.args[0], tree, stack);

			if (TYPE_F32 == ret.type)
				fprintf(stdout, "%f\n", ret.f32);
			else
				fprintf(stdout, "%d\n", ret.i32);

			return ret;
		case INTRIN_READ_I32:
			return Value{ .type = TYPE_I32, .i32 = read<int32_t>("i: ", "%d") };
		case INTRIN_READ_F32:
			return Value{ .type = TYPE_F32, .f32 = read<float>("f: ", "%f") };
		default:
			// it's a defun -- collect all args, pushing them onto the var stack
			for (ASTNodeIndices::const_iterator it = node.args.begin(); it != node.args.end(); ++it) {
				const Value arg = eval(*it, tree, stack);
				stack.push_back(NamedValue{ .name = nullidx, .val = arg });
			}
			// invoke the callee, which will pop the var stack once done
			return eval(node.eval, tree, stack, node.args.size());
		}
	case ASTNODE_LITERAL_I32:
		return Value{ .type = TYPE_I32, .i32 = node.literal_i32 };
	case ASTNODE_LITERAL_F32:
		return Value{ .type = TYPE_F32, .f32 = node.literal_f32 };
	}

	assert(false);
	return Value{ .type = TYPE_F32, .f32 = 0.f / 0 };
}

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

	if (!tokenize(buffer.data(), tokens)) {
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
	const ASTNode root = { .name = { .ptr = nullptr, .len = 0 }, .type = ASTNODE_LET, .parent = nullidx, .args = ASTNodeIndices() };
	tree.push_back(root);

	// collect top-level expressions/statements, registering them as root sub-nodes
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

	// root expression must return something
	if (0 == getSubCount(false, 0, tree)) {
		fprintf(stderr, "root expression does not return\nfailure\n");
		return -1;
	}

	// no use of printing the dummy root node -- print its sub-nodes instead
	for (ASTNodeIndices::const_iterator it = tree.front().args.begin(); it != tree.front().args.end(); ++it)
		tree[*it].print(stdout, tree, 0);

	fprintf(stdout, "success\n");

	// evaluate AST and print result
	VarStack stack;
	const Value res = eval(0, tree, stack);
	res.print(stdout);

	assert(stack.empty());
	return 0;
}
