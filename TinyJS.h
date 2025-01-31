/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *

 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored / Changed By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2015 ardisoft
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TINYJS_H
#define TINYJS_H


#define TINY_JS_VERSION 0.9.9

#include <string>
#include <vector>
#include <map>
#include <set>
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || _MSC_VER >= 1700 // Visual Studio 2012
#	include <cstdint>
#else
#	define __STDC_LIMIT_MACROS
#	include <stdint.h> // <cstdint> is C++11
#endif
#include <climits>
#include <cstring>
#include <cassert>
#include <ctime>
#include <limits>
#include <iostream>

#include "config.h"

#ifdef NO_POOL_ALLOCATOR
	template<typename T, int num_objects=64>
	class fixed_size_object {};
#else
#	include "pool_allocator.h"
#endif
#include "TinyJS_Threading.h"


#ifdef _MSC_VER
#	if defined(_DEBUG) && defined(_DEBUG_NEW)
#		define _AFXDLL
#		include <afx.h>         // MFC-Kern- und -Standardkomponenten
#		define new DEBUG_NEW
#	endif
#	define DEPRECATED(_Text) __declspec(deprecated(_Text))
#	define ATTRIBUTE_USED
#elif defined(__GNUC__)
#	define DEPRECATED(_Text) __attribute__ ((deprecated))
#	define ATTRIBUTE_USED __attribute__((used))
#else
#	define DEPRECATED(_Text)
#	define ATTRIBUTE_USED
#endif

#ifdef NO_WARN_DEPRECATED
#	undef DEPRECATED
#	define DEPRECATED(_Text)
#endif

#ifndef ASSERT
#	define ASSERT(X) assert(X)
#endif

#undef TRACE
#ifndef TRACE
#define TRACE printf
#endif // TRACE

/*!
 *  indicates the highest supported version of compiled js
 *  when enum LEX_TYPES are changed, then increment this version
 *  compiled js are created with this version
 */
#define COMPILED_TOKENS_VERSION_MAX 0x0101
/*!
 *  indicates the lowest supported version of compiled js
 *  when id's inserted, removed or reordered, then set version min to version max
 */
#define COMPILED_TOKENS_VERSION_MIN 0x0101

enum LEX_TYPES {
	LEX_EOF = 0,

	// reserved words
	LEX_R_IF = 256,
	LEX_R_ELSE,
	LEX_R_DO,
	LEX_R_WHILE,
	LEX_R_FOR,
	LEX_R_IN,
	LEX_T_OF,
	LEX_R_BREAK,
	LEX_R_CONTINUE,
	LEX_R_RETURN,
	LEX_R_VAR,
	LEX_R_LET,
	LEX_R_CONST,
	LEX_R_WITH,
	LEX_R_TRUE,
	LEX_R_FALSE,
	LEX_R_NULL,
	LEX_R_NEW,
	LEX_R_TRY,
	LEX_R_CATCH,
	LEX_R_FINALLY,
	LEX_R_THROW,
	LEX_R_TYPEOF,
	LEX_R_VOID,
	LEX_R_DELETE,
	LEX_R_INSTANCEOF,
	LEX_R_SWITCH,
	LEX_R_CASE,
	LEX_R_DEFAULT,
	LEX_R_YIELD,

#define LEX_EQUALS_BEGIN LEX_EQUAL
	LEX_EQUAL,					// ==
	LEX_TYPEEQUAL,				// ===
	LEX_NEQUAL,					// !=
	LEX_NTYPEEQUAL,				// !==
#define LEX_EQUALS_END LEX_NTYPEEQUAL

	LEX_ARROW,					// =>
	LEX_LEQUAL,					// <=
	LEX_GEQUAL,					// >=

#define LEX_SHIFTS_BEGIN LEX_LSHIFT
	LEX_LSHIFT,					// <<
	LEX_RSHIFT,					// >>
	LEX_RSHIFTU,				// >>> (unsigned)
#define LEX_SHIFTS_END LEX_RSHIFTU

	LEX_ASKASK,					// ??
	LEX_ASTERISKASTERISK,		// **
	LEX_PLUSPLUS,				// ++
	LEX_MINUSMINUS,				// --
	LEX_ANDAND,					// &&
	LEX_OROR,					// ||
	LEX_INT,

#define LEX_ASSIGNMENTS_BEGIN LEX_PLUSEQUAL
	LEX_PLUSEQUAL,				// +=
	LEX_MINUSEQUAL,				// -=
	LEX_ASTERISKEQUAL,			// *=
	LEX_ASTERISKASTERISKEQUAL,	// **=
	LEX_SLASHEQUAL,				// /=
	LEX_PERCENTEQUAL,			// %=
	LEX_LSHIFTEQUAL,			// <<=
	LEX_RSHIFTEQUAL,			// >>=
	LEX_RSHIFTUEQUAL,			// >>>= (unsigned)
	LEX_ANDEQUAL,				// &=
	LEX_OREQUAL,				// |=
	LEX_XOREQUAL,				// ^=
	LEX_ASKASKEQUAL,			// ??=
#define LEX_ASSIGNMENTS_END LEX_ASKASKEQUAL



#define LEX_TOKEN_NONSIMPLE_BEGIN LEX_TOKEN_STRING_BEGIN // tokens with a special CScriptTokenData class

#define LEX_TOKEN_STRING_BEGIN LEX_ID			// CScriptTokenDataString
	LEX_ID,
	LEX_STR,
	LEX_REGEXP,
	LEX_T_LABEL,
	LEX_T_DUMMY_LABEL,
#define LEX_TOKEN_STRING_END LEX_T_DUMMY_LABEL

	LEX_FLOAT,
#define LEX_TOKEN_NONSIMPLE_1_END LEX_FLOAT					// float


	// special token

#define LEX_TOKEN_FOR_BEGIN LEX_T_LOOP						// CScriptTokenDataLoop
	LEX_T_LOOP,
	LEX_T_FOR_IN,
#define LEX_TOKEN_FOR_END LEX_T_FOR_IN
#define LEX_TOKEN_FUNCTION_BEGIN LEX_R_FUNCTION				// CScriptTokenDataFnc
	LEX_R_FUNCTION,
	LEX_T_FUNCTION_PLACEHOLDER,
	LEX_T_FUNCTION_OPERATOR,
	LEX_T_FUNCTION_ARROW,
	LEX_T_GENERATOR,
	LEX_T_GENERATOR_OPERATOR,
	LEX_T_GENERATOR_MEMBER,
	LEX_T_GET,
	LEX_T_SET,
#define LEX_TOKEN_FUNCTION_END LEX_T_SET
	LEX_T_IF,												// CScriptTokenDataIf
	LEX_T_TRY,												// CScriptTokenDataTry
	LEX_T_OBJECT_LITERAL,									// CScriptTokenDataObjectLiteral
	LEX_T_DESTRUCTURING_VAR,								// CScriptTokenDataDestructuringVar
	LEX_T_ARRAY_COMPREHENSIONS_BODY,						// CScriptTokenDataArrayComprehensionsBody
	LEX_T_FORWARD,											// CScriptTokenDataForwards
#define LEX_TOKEN_NONSIMPLE_END LEX_T_FORWARD

	LEX_T_EXCEPTION_VAR,
	LEX_T_SKIP,
	LEX_T_END_EXPRESSION,

	LEX_OPTIONAL_CHAINING_MEMBER,	// .?
	LEX_OPTIONAL_CHAINING_ARRAY,	// .?[ ... ]
	LEX_OPTIONAL_CHANING_FNC,		// .?( ... )

};
#define LEX_TOKEN_DATA_STRING(tk)							((LEX_TOKEN_STRING_BEGIN<= tk && tk <= LEX_TOKEN_STRING_END))
#define LEX_TOKEN_DATA_FLOAT(tk)							(tk==LEX_FLOAT)
#define LEX_TOKEN_DATA_LOOP(tk)								(LEX_TOKEN_FOR_BEGIN <= tk && tk <= LEX_TOKEN_FOR_END)
#define LEX_TOKEN_DATA_FUNCTION(tk)							(LEX_TOKEN_FUNCTION_BEGIN <= tk && tk <= LEX_TOKEN_FUNCTION_END)
#define LEX_TOKEN_DATA_IF(tk)								(tk==LEX_T_IF)
#define LEX_TOKEN_DATA_TRY(tk)								(tk==LEX_T_TRY)
#define LEX_TOKEN_DATA_OBJECT_LITERAL(tk)					(tk==LEX_T_OBJECT_LITERAL)
#define LEX_TOKEN_DATA_DESTRUCTURING_VAR(tk)				(tk==LEX_T_DESTRUCTURING_VAR)
#define LEX_TOKEN_DATA_ARRAY_COMPREHENSIONS_BODY(tk)		(tk==LEX_T_ARRAY_COMPREHENSIONS_BODY)
#define LEX_TOKEN_DATA_FORWARDER(tk)						(tk==LEX_T_FORWARD)

#define LEX_TOKEN_DATA_SIMPLE(tk) (!(LEX_TOKEN_NONSIMPLE_BEGIN <= tk && tk <= LEX_TOKEN_NONSIMPLE_END))

enum SCRIPTVARLINK_FLAGS {
	SCRIPTVARLINK_WRITABLE			= 1<<0,
	SCRIPTVARLINK_CONFIGURABLE		= 1<<1,
	SCRIPTVARLINK_ENUMERABLE		= 1<<2,
	SCRIPTVARLINK_DEFAULT			= SCRIPTVARLINK_WRITABLE | SCRIPTVARLINK_CONFIGURABLE | SCRIPTVARLINK_ENUMERABLE,
	SCRIPTVARLINK_VARDEFAULT		= SCRIPTVARLINK_WRITABLE | SCRIPTVARLINK_ENUMERABLE,
	SCRIPTVARLINK_CONSTDEFAULT		= SCRIPTVARLINK_ENUMERABLE,
	SCRIPTVARLINK_BUILDINDEFAULT	= SCRIPTVARLINK_WRITABLE | SCRIPTVARLINK_CONFIGURABLE,
	SCRIPTVARLINK_READONLY			= SCRIPTVARLINK_CONFIGURABLE,
	SCRIPTVARLINK_READONLY_ENUM	= SCRIPTVARLINK_CONFIGURABLE | SCRIPTVARLINK_ENUMERABLE,
	SCRIPTVARLINK_CONSTANT			= 0,
};

enum ERROR_TYPES {
	Error = 0,
	EvalError,
	RangeError,
	ReferenceError,
	SyntaxError,
	TypeError
};
#define ERROR_MAX TypeError
#define ERROR_COUNT (ERROR_MAX+1)
extern const char *ERROR_NAME[];

#define TEMPORARY_MARK_SLOTS 5

#define TINYJS_RETURN_VAR					"return"
#define TINYJS_LOKALE_VAR					"__locale__"
#define TINYJS_ANONYMOUS_VAR				"__anonymous__"
#define TINYJS_ARGUMENTS_VAR				"arguments"
#define TINYJS_PROTOTYPE_CLASS			"prototype"
#define TINYJS_FUNCTION_CLOSURE_VAR		"__function_closure__"
#define TINYJS_SCOPE_PARENT_VAR			"__scope_parent__"
#define TINYJS_SCOPE_WITH_VAR				"__scope_with__"
#define TINYJS_ACCESSOR_GET_VAR			"__accessor_get__"
#define TINYJS_ACCESSOR_SET_VAR			"__accessor_set__"
#define TINYJS_CONSTRUCTOR_VAR			"constructor"
#define TINYJS_TEMP_NAME			""
#define TINYJS_BLANK_DATA			""

typedef std::vector<std::string> STRING_VECTOR_t;
typedef STRING_VECTOR_t::iterator STRING_VECTOR_it;
typedef STRING_VECTOR_t::const_iterator STRING_VECTOR_cit;

typedef std::set<std::string> STRING_SET_t;
typedef STRING_SET_t::iterator STRING_SET_it;

/// convert the given string into a quoted string suitable for javascript
std::string getJSString(const std::string &str);
/// convert the given int into a string

// GCC 4.8 and above because issue with MingW
#if (__cplusplus >= 201103L || isCXX0x(4,8) || _MSC_VER >= 1600) // Visual Studio 2010 and above
#	define HAVE_STD_TO_STRING 1
	inline std::string int2string(long long intData) { return std::to_string(intData); }
	inline std::string int2string(unsigned long long intData) { return std::to_string(intData); }
#	if !defined(_MSC_VER) || _MSC_VER >= 19800// Visual Studio 2013 and above
		template<typename intType>
		typename std::enable_if<std::is_integral<intType>::value, std::string>::type int2string(intType intData) { return std::to_string(intData); }
#	else
		template<typename intType>
		typename std::enable_if<std::is_integral<intType>::value && std::is_signed<intType>::value, std::string>::type int2string(intType intData) { return std::to_string((long long)intData); }
		template<typename intType>
		typename std::enable_if<std::is_integral<intType>::value && std::is_unsigned<intType>::value, std::string>::type int2string(intType intData) { return std::to_string((unsigned long long)intData); }
#	endif
#else
// 	std::string int2string(int intData);
// 	std::string int2string(unsigned intData);
// 	std::string int2string(long intData);
// 	std::string int2string(unsigned long intData);
// 	std::string int2string(long long intData);
// 	std::string int2string(unsigned long long intData);
	template<typename intType>
	std::string int2string(intType intData);

#endif


/// convert the given double into a string
std::string float2string(const double &floatData);


//////////////////////////////////////////////////////////////////////////
/// CScriptException
//////////////////////////////////////////////////////////////////////////

class CScriptException {
public:
	ERROR_TYPES errorType;
	std::string message;
	std::string fileName;
	int32_t lineNumber;
	int32_t column;
	CScriptException(const std::string &Message, const std::string &File, int32_t Line=-1, int32_t Column=-1) :
						errorType(Error), message(Message), fileName(File), lineNumber(Line), column(Column){}
	CScriptException(ERROR_TYPES ErrorType, const std::string &Message, const std::string &File, int32_t Line=-1, int32_t Column=-1) :
						errorType(ErrorType), message(Message), fileName(File), lineNumber(Line), column(Column){}
	CScriptException(const std::string &Message, const char *File="", int32_t Line=-1, int32_t Column=-1) :
						errorType(Error), message(Message), fileName(File), lineNumber(Line), column(Column){}
	CScriptException(ERROR_TYPES ErrorType, const std::string &Message, const char *File="", int32_t Line=-1, int32_t Column=-1) :
						errorType(ErrorType), message(Message), fileName(File), lineNumber(Line), column(Column){}
	std::string toString();
};


//////////////////////////////////////////////////////////////////////////
/// CScriptLex
//////////////////////////////////////////////////////////////////////////

class CScriptLex
{
public:
	CScriptLex(const char* Code, const std::string& File = "", int Line = 0, int Column = 0);
	struct POS;
	int tk; ///< The type of the token that we have
	int last_tk; ///< The type of the last token that we have
	std::string tkStr; ///< Data contained in the token we have here

	void check(int expected_tk, int alternate_tk=-1); ///< Lexical check wotsit
	void match(int expected_tk, int alternate_tk=-1); ///< Lexical match wotsit
	void reset(const POS &toPos); ///< Reset this lex so we can start again
	const char* rest() const { return pos.tokenStart; }
	std::string currentFile;
	struct POS {
		const char *tokenStart;
		int32_t currentLine;
		const char *currentLineStart;
		int16_t currentColumn() const { return (int16_t)(tokenStart - currentLineStart) /* silently casted to int16_t because always checked in match(...) */; }
	} pos;
	int32_t currentLine() const { return pos.currentLine; }
	int16_t currentColumn() const { return pos.currentColumn(); }
	bool lineBreakBeforeToken;
private:
	const char *data;
	const char *dataPos;
	char currCh, nextCh;

	void getNextCh();
	void getNextToken(); ///< Get the text token from our text string
};


//////////////////////////////////////////////////////////////////////////
/// CScriptTokenData
//////////////////////////////////////////////////////////////////////////

class CScriptToken;
typedef  std::vector<CScriptToken> TOKEN_VECT;
typedef  std::vector<CScriptToken>::iterator TOKEN_VECT_it;
typedef  std::vector<CScriptToken>::const_iterator TOKEN_VECT_cit;

class CScriptTokenData
{
protected:
	CScriptTokenData() : refs(0){}
	virtual ~CScriptTokenData() {}
private:
//	CScriptTokenData(const CScriptTokenData &noCopy);
//	CScriptTokenData &operator=(const CScriptTokenData &noCopy);
public:
	void ref() { refs++; }
	void unref() { if(--refs == 0) delete this; }
	virtual void serialize(std::ostream &) const=0;
private:
	int refs;
};
template<typename C>
class CScriptTokenDataPtr {
public:
	CScriptTokenDataPtr() : ptr(0) {}
	CScriptTokenDataPtr(const CScriptTokenDataPtr &Copy) : ptr(0) { *this=Copy; }
	CScriptTokenDataPtr &operator=(const CScriptTokenDataPtr &Copy) {
		if(ptr != Copy.ptr) {
			if(ptr) ptr->unref();
			if((ptr = Copy.ptr)) ptr->ref();
		}
		return *this;
	}
	CScriptTokenDataPtr(C &Init) { (ptr=&Init)->ref(); }
	~CScriptTokenDataPtr() { if(ptr) ptr->unref(); }
	C *operator->() { return ptr; }
	const C *operator->() const { return ptr; }
	C &operator*() { return *ptr; }
	operator bool() const { return ptr!=0; }
	bool operator==(const CScriptTokenDataPtr& rhs) const { return ptr==rhs.ptr; }
private:
	C *ptr;
};

class CScriptTokenDataString : public fixed_size_object<CScriptTokenDataString>, public CScriptTokenData {
public:
	CScriptTokenDataString() {}
	CScriptTokenDataString(const std::string &String) : tokenStr(String) {}
	CScriptTokenDataString(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;
	std::string tokenStr;
private:
};

class CScriptTokenDataFnc : public fixed_size_object<CScriptTokenDataFnc>, public CScriptTokenData {
public:
	CScriptTokenDataFnc(int32_t Type) : type(Type), line(0) {}
	CScriptTokenDataFnc(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;
	std::string getArgumentsString(bool forArrowFunction=false);

	int32_t type;
	std::string file;
	int32_t line;
	std::string name;
	TOKEN_VECT arguments;
	TOKEN_VECT body;
	bool isGenerator() { return type == LEX_T_GENERATOR || type == LEX_T_GENERATOR_OPERATOR || type == LEX_T_GENERATOR_MEMBER; }
	bool isArrowFunction() { return type == LEX_T_FUNCTION_ARROW; }

};
typedef CScriptTokenDataPtr<CScriptTokenDataFnc> CScriptTokenDataFncPtr;

class CScriptTokenDataForwards : public fixed_size_object<CScriptTokenDataForwards>, public CScriptTokenData {
public:
	CScriptTokenDataForwards() {}
	CScriptTokenDataForwards(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;

	bool checkRedefinition(const std::string &Str, bool checkVars);
	void addVars( STRING_VECTOR_t &Vars );
	void addConsts( STRING_VECTOR_t &Vars );
	std::string addVarsInLetscope(STRING_VECTOR_t &Vars);
	std::string addLets(STRING_VECTOR_t &Lets);
	bool empty() const { return varNames[LETS].empty() && varNames[VARS].empty() && varNames[CONSTS].empty() && functions.empty(); }

	enum {
		LETS = 0,
		VARS,
		CONSTS,
		END
	};
	STRING_SET_t varNames[END];
	STRING_SET_t vars_in_letscope;
	class compare_fnc_token_by_name {
	public:
		bool operator()(const CScriptToken& lhs, const CScriptToken& rhs) const;
	};
	typedef std::set<CScriptToken, compare_fnc_token_by_name> FNC_SET_t;
	typedef FNC_SET_t::iterator FNC_SET_it;
	FNC_SET_t functions;

private:
};

#ifdef old
class CScriptTokenDataForwardsPtr {
public:
	CScriptTokenDataForwardsPtr() : ptr(0) {}
	CScriptTokenDataForwardsPtr(const CScriptTokenDataForwardsPtr &Copy) : ptr(0) { *this=Copy; }
	CScriptTokenDataForwardsPtr &operator=(const CScriptTokenDataForwardsPtr &Copy) {
		if(ptr != Copy.ptr) {
			if(ptr) ptr->unref();
			if((ptr = Copy.ptr)) ptr->ref();
		}
		return *this;
	}
	CScriptTokenDataForwardsPtr(CScriptTokenDataForwards &Init) { (ptr=&Init)->ref(); }
	~CScriptTokenDataForwardsPtr() { if(ptr) ptr->unref(); }
	CScriptTokenDataForwards *operator->() { return ptr; }
	operator bool() { return ptr!=0; }
	bool operator==(const CScriptTokenDataForwardsPtr& rhs) { return ptr==rhs.ptr; }

private:
	CScriptTokenDataForwards *ptr;
};
#else
typedef CScriptTokenDataPtr<CScriptTokenDataForwards> CScriptTokenDataForwardsPtr;
#endif

typedef std::vector<CScriptTokenDataForwardsPtr> FORWARDER_VECTOR_t;

class CScriptTokenDataLoop : public fixed_size_object<CScriptTokenDataLoop>, public CScriptTokenData {
public:
	CScriptTokenDataLoop() { type=FOR; }
	CScriptTokenDataLoop(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;

	std::string getParsableString(const std::string &IndentString="", const std::string &Indent="");

	enum {FOR_EACH=0, FOR_IN, FOR_OF, FOR, WHILE, DO} type; // do not change the order
	STRING_VECTOR_t labels;
	TOKEN_VECT init;
	TOKEN_VECT condition;
	TOKEN_VECT iter;
	TOKEN_VECT body;
};

class CScriptTokenDataIf : public fixed_size_object<CScriptTokenDataIf>, public CScriptTokenData {
public:
	CScriptTokenDataIf() {}
	CScriptTokenDataIf(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;
	std::string getParsableString(const std::string &IndentString="", const std::string &Indent="");
	TOKEN_VECT condition;
	TOKEN_VECT if_body;
	TOKEN_VECT else_body;
};

typedef std::pair<std::string, std::string> DESTRUCTURING_VAR_t;
typedef std::vector<DESTRUCTURING_VAR_t> DESTRUCTURING_VARS_t;
typedef DESTRUCTURING_VARS_t::iterator DESTRUCTURING_VARS_it;
typedef DESTRUCTURING_VARS_t::const_iterator DESTRUCTURING_VARS_cit;
class CScriptTokenDataDestructuringVar : public fixed_size_object<CScriptTokenDataDestructuringVar>, public CScriptTokenData {
public:
	CScriptTokenDataDestructuringVar() {}
	CScriptTokenDataDestructuringVar(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;
	std::string getParsableString();

	void getVarNames(STRING_VECTOR_t &Names);

	DESTRUCTURING_VARS_t vars;
	TOKEN_VECT assignment;
private:
};

class CScriptTokenDataObjectLiteral : public fixed_size_object<CScriptTokenDataObjectLiteral>, public CScriptTokenData {
public:
	CScriptTokenDataObjectLiteral() : type(CScriptTokenDataObjectLiteral::OBJECT), destructuring(false), structuring(false) {}
	CScriptTokenDataObjectLiteral(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;

	std::string getParsableString();

	void setMode(bool Destructuring);
	bool toDestructuringVar(CScriptTokenDataDestructuringVar &DestructuringVar);

	enum {OBJECT, ARRAY, ARRAY_COMPREHENSIONS, ARRAY_COMPREHENSIONS_OLD} type;
	struct ELEMENT {
		std::string id;
		TOKEN_VECT value;
	};
	bool destructuring;
	bool structuring;
	typedef std::vector<ELEMENT> ELEMENTS_t;
	typedef ELEMENTS_t::iterator ELEMENTS_it;
	typedef ELEMENTS_t::const_iterator ELEMENTS_cit;
	ELEMENTS_t elements;

private:
};

class CScriptTokenDataArrayComprehensionsBody : public fixed_size_object<CScriptTokenDataArrayComprehensionsBody>, public CScriptTokenData {
public:
	CScriptTokenDataArrayComprehensionsBody() {}
	CScriptTokenDataArrayComprehensionsBody(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;

	TOKEN_VECT body;
};


class CScriptTokenDataTry : public fixed_size_object<CScriptTokenDataTry>, public CScriptTokenData {
public:
	CScriptTokenDataTry() {}
	CScriptTokenDataTry(std::istream &in);
	virtual void serialize(std::ostream &out) const OVERRIDE;

	std::string getParsableString(const std::string &IndentString="", const std::string &Indent="");

	TOKEN_VECT tryBlock;
	struct CatchBlock {
		CScriptTokenDataPtr<CScriptTokenDataDestructuringVar> indentifiers;
		TOKEN_VECT condition;
		TOKEN_VECT block;
	};
	typedef std::vector<CatchBlock> CATCHBLOCKS_t;
	typedef CATCHBLOCKS_t::iterator CATCHBLOCKS_it;
	typedef CATCHBLOCKS_t::const_iterator CATCHBLOCKS_cit;
	CATCHBLOCKS_t catchBlocks;
	TOKEN_VECT finallyBlock;
};


//////////////////////////////////////////////////////////////////////////
/// CScriptToken
//////////////////////////////////////////////////////////////////////////

class CScriptTokenizer;
/*
	a Token needs 8 Byte
	2 Bytes for the Row-Position of the Token
	2 Bytes for the Token self
	and
	4 Bytes for special Datas in an union
			e.g. an int for interger-literals
			or pointer for double-literals,
			for string-literals or for functions
*/
class CScriptToken : public fixed_size_object<CScriptToken>
{
public:
	CScriptToken() : line(0), column(0), token(0), intData(0) {}
	CScriptToken(CScriptLex *l, int Match=-1, int Alternate=-1);
	CScriptToken(uint16_t Tk, int32_t IntData=0);
	CScriptToken(uint16_t Tk, double FloatData);
	CScriptToken(uint16_t Tk, const std::string &TkStr);
	CScriptToken(const  CScriptToken &Copy) : token(0) { *this = Copy; }
	CScriptToken &operator =(const CScriptToken &Copy);
	CScriptToken(std::istream &in);
	~CScriptToken() { clear(); }

	void serialize(std::ostream &out) const;

	int32_t &Int() { ASSERT(LEX_TOKEN_DATA_SIMPLE(token)); return intData; }
	std::string &String() { ASSERT(LEX_TOKEN_DATA_STRING(token)); return dynamic_cast<CScriptTokenDataString*>(tokenData)->tokenStr; }
	double &Float() { ASSERT(LEX_TOKEN_DATA_FLOAT(token)); return *floatData; }
	CScriptTokenDataFnc &Fnc() { ASSERT(LEX_TOKEN_DATA_FUNCTION(token)); return *dynamic_cast<CScriptTokenDataFnc*>(tokenData); }
	const CScriptTokenDataFnc &Fnc() const { ASSERT(LEX_TOKEN_DATA_FUNCTION(token)); return *dynamic_cast<CScriptTokenDataFnc*>(tokenData); }
	CScriptTokenDataObjectLiteral &Object() { ASSERT(LEX_TOKEN_DATA_OBJECT_LITERAL(token)); return *dynamic_cast<CScriptTokenDataObjectLiteral*>(tokenData); }
	CScriptTokenDataDestructuringVar &DestructuringVar() { ASSERT(LEX_TOKEN_DATA_DESTRUCTURING_VAR(token)); return *dynamic_cast<CScriptTokenDataDestructuringVar*>(tokenData); }
	CScriptTokenDataArrayComprehensionsBody &ArrayComprehensionsBody() { ASSERT(LEX_TOKEN_DATA_ARRAY_COMPREHENSIONS_BODY(token)); return *dynamic_cast<CScriptTokenDataArrayComprehensionsBody*>(tokenData); }
	CScriptTokenDataLoop &Loop() { ASSERT(LEX_TOKEN_DATA_LOOP(token)); return *dynamic_cast<CScriptTokenDataLoop*>(tokenData); }
	CScriptTokenDataIf &If() { ASSERT(LEX_TOKEN_DATA_IF(token)); return *dynamic_cast<CScriptTokenDataIf*>(tokenData); }
	CScriptTokenDataTry &Try() { ASSERT(LEX_TOKEN_DATA_TRY(token)); return *dynamic_cast<CScriptTokenDataTry*>(tokenData); }
	CScriptTokenDataForwards &Forwarder() { ASSERT(LEX_TOKEN_DATA_FORWARDER(token)); return *dynamic_cast<CScriptTokenDataForwards*>(tokenData); }
	CScriptTokenData &TokenData() { CScriptTokenData *data = dynamic_cast<CScriptTokenData*>(tokenData); ASSERT(data); return *data; }
	const CScriptTokenData &TokenData() const { CScriptTokenData *data = dynamic_cast<CScriptTokenData*>(tokenData); ASSERT(data); return *data; }
#ifdef _DEBUG
	std::string token_str;
#endif
	uint16_t			line;
	uint16_t			column;
	uint16_t			token;

	// ACTUAL_CHANGE
	static std::string getParsableString(TOKEN_VECT &Tokens, const std::string &IndentString="", const std::string &Indent="");
	static std::string getParsableString(TOKEN_VECT_it Begin, TOKEN_VECT_it End, const std::string &IndentString="", const std::string &Indent="");
	static std::string getTokenStr( int token, const char *tokenStr=0, bool *need_space=0 );
	static const char *isReservedWord(int Token);
	static int isReservedWord(const std::string &Str);

	template<typename T> static void serialize(const T &value, std::ostream &out) {
		out.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}
	template<typename T> static T &unserialize(T &value, std::istream &in) {
		in.read(reinterpret_cast<char*>(&value), sizeof(value));
		return value;
	}
	static void serialize(const std::string &value, std::ostream &out);
	static std::string &unserialize(std::string &value, std::istream &in);

	static void serialize(const STRING_VECTOR_t &value, std::ostream &out);
	static STRING_VECTOR_t &unserialize(STRING_VECTOR_t &value, std::istream &in);

	static void serialize(const TOKEN_VECT &Tokens, std::ostream &out);
	static void unserialize(TOKEN_VECT &Tokens, std::istream &in);

private:
	void clear();
	union {
		int32_t									intData;
		double									*floatData;
		CScriptTokenData						*tokenData;
	};
};


//////////////////////////////////////////////////////////////////////////
/// CScriptTokenizer - converts the code in a vector with tokens
//////////////////////////////////////////////////////////////////////////

typedef std::vector<size_t> MARKS_t;

class CScriptTokenizer
{
public:
	struct ScriptTokenPosition {
		ScriptTokenPosition(TOKEN_VECT *Tokens) : tokens(Tokens), pos(tokens->begin())/*, currentLine(0)*//*, currentColumn(0)*/ {}
		bool operator ==(const ScriptTokenPosition &eq) { return pos == eq.pos; }
		ScriptTokenPosition &operator =(const ScriptTokenPosition &copy) {
			tokens=copy.tokens; pos=copy.pos;
			return *this;
		}
		TOKEN_VECT *tokens;
		TOKEN_VECT_it pos;
		int currentLine() const { return pos->line; }
		int currentColumn() const { return pos->column; }
	};
	struct ScriptTokenState {
		ScriptTokenState() : LeftHand(false), /*FunctionIsGenerator(false),*/ HaveReturnValue(false) {}
		TOKEN_VECT Tokens;
		FORWARDER_VECTOR_t Forwarders;
		MARKS_t Marks;
		STRING_VECTOR_t Labels;
		STRING_VECTOR_t LoopLabels;
		bool LeftHand;
		void pushLeftHandState() { States.push_back(LeftHand); }
		void popLeftHandeState() { LeftHand = States.back(); States.pop_back(); }
		std::vector<bool> States;
//		bool FunctionIsGenerator;
		bool HaveReturnValue;
	};
	CScriptTokenizer();
	CScriptTokenizer(CScriptLex &Lexer);
	CScriptTokenizer(const char *Code, const std::string &File="", int Line=0, int Column=0);
	static bool writeCompiledTokens;
private:
	void unserialize(const std::string &File, const std::string &FileC="");
	void serialize(std::ostream &out) const;
	void serialize(const std::string &File);
	void serialize(const std::string &File, const std::nothrow_t &);
public:

	void tokenizeCode(CScriptLex &Lexer);

	CScriptToken &getToken() { return *(tokenScopeStack.back().pos); }
	void getNextToken();
	bool check(int ExpectedToken, int AlternateToken=-1);
	void match(int ExpectedToken, int AlternateToken=-1);
	void pushTokenScope(TOKEN_VECT &Tokens);
	ScriptTokenPosition &getPos() { return tokenScopeStack.back(); }
	void setPos(ScriptTokenPosition &TokenPos);
	ScriptTokenPosition &getPrevPos() { return prevPos; }
	void skip(int Tokens);
	int tk; // current Token
	std::string currentFile;
	int currentLine() { return getPos().currentLine();}
	int currentColumn() { return getPos().currentColumn();}
	const std::string &tkStr() { static std::string empty; return LEX_TOKEN_DATA_STRING(getToken().token)?getToken().String():empty; }
private:
	void tokenizeTry(ScriptTokenState &State, int Flags);
	void tokenizeSwitch(ScriptTokenState &State, int Flags);
	void tokenizeWith(ScriptTokenState &State, int Flags);
	void tokenizeWhileAndDo(ScriptTokenState &State, int Flags);
	void tokenizeIf_inArrayComprehensions(ScriptTokenState &State, int Flags, TOKEN_VECT &Assign);
	void tokenizeIf(ScriptTokenState &State, int Flags);
	void tokenizeFor_inArrayComprehensions(ScriptTokenState &State, int Flags, TOKEN_VECT &Assign);
	void tokenizeFor(ScriptTokenState &State, int Flags);
	CScriptToken tokenizeVarIdentifier(STRING_VECTOR_t *VarNames=0, bool *NeedAssignment=0);
	CScriptToken tokenizeFunctionArgument();
	void tokenizeArrowFunction(const TOKEN_VECT &Arguments, ScriptTokenState &State, int Flags, bool noLetDef=false);
	void tokenizeFunction(ScriptTokenState &State, int Flags, bool noLetDef=false);
	void tokenizeLet(ScriptTokenState &State, int Flags, bool noLetDef=false);
	void tokenizeVarNoConst(ScriptTokenState &State, int Flags);
	void tokenizeVarAndConst(ScriptTokenState &State, int Flags);
	void _tokenizeLiteralObject(ScriptTokenState &State, int Flags);
	void _tokenizeLiteralArray(ScriptTokenState &State, int Flags);
	bool _tokenizeArrayComprehensions(ScriptTokenState &State, int Flags);

	void tokenizeLiteral(ScriptTokenState &State, int Flags);
	void tokenizeMember(ScriptTokenState &State, int Flags);
	void tokenizeFunctionCall(ScriptTokenState &State, int Flags);
	void tokenizeSubExpression(ScriptTokenState &State, int Flags);
	void tokenizeLogic(ScriptTokenState &State, int Flags, int op= LEX_OROR, int op_n=LEX_ANDAND);
	void tokenizeCondition(ScriptTokenState &State, int Flags);
	void tokenizeAssignment(ScriptTokenState& State, int Flags);	// = += -= *= /= %= <<= >>= >>>= &= |= ^= AND ??=
	void tokenizeExpression(ScriptTokenState& State, int Flags);	// ..., ... 
	void tokenizeBlock(ScriptTokenState& State, int Flags);			// { ... }
	void tokenizeStatementNoLet(ScriptTokenState &State, int Flags);
	void tokenizeStatement(ScriptTokenState &State, int Flags);

	size_t pushToken(TOKEN_VECT &Tokens, int Match=-1, int Alternate=-1);
	size_t pushToken(TOKEN_VECT &Tokens, const CScriptToken &Token);
	void pushForwarder(ScriptTokenState &State, bool noMarks=false);
	void removeEmptyForwarder(ScriptTokenState &State);
	void pushForwarder(TOKEN_VECT &Tokens, FORWARDER_VECTOR_t &Forwarders, MARKS_t &Marks);
	void removeEmptyForwarder(TOKEN_VECT &Tokens, FORWARDER_VECTOR_t &Forwarders, MARKS_t &Marks);
	void throwTokenNotExpected();
	CScriptLex *l;
	TOKEN_VECT tokens;
	ScriptTokenPosition prevPos;
	std::vector<ScriptTokenPosition> tokenScopeStack;
};


//////////////////////////////////////////////////////////////////////////
/// forward-declaration
//////////////////////////////////////////////////////////////////////////

class CNumber;
class CScriptVar;
class CScriptVarPtr;
template<typename C> class CScriptVarPointer;
class CScriptVarLink;
class CScriptVarLinkPtr;
class CScriptVarLinkWorkPtr;

class CScriptVarPrimitive;
typedef CScriptVarPointer<CScriptVarPrimitive> CScriptVarPrimitivePtr;

class CScriptVarScopeFnc;
typedef CScriptVarPointer<CScriptVarScopeFnc> CFunctionsScopePtr;
typedef void (*JSCallback)(const CFunctionsScopePtr &var, void *userdata);

class CTinyJS;
class CScriptResult;

enum IteratorMode {
	RETURN_KEY = 1,
	RETURN_VALUE = 2,
	RETURN_ARRAY = 3
};

//////////////////////////////////////////////////////////////////////////
/// CScriptPropertyName
//////////////////////////////////////////////////////////////////////////

/// CScriptPropertyName holds the name of any property
class CScriptPropertyName {
public:
	CScriptPropertyName() : idx(-1) {}
	// property name
	CScriptPropertyName(const std::string &Name) : name(Name), idx(name2arrayIdx(Name)) {}
	// array index
	CScriptPropertyName(uint32_t Idx) : name(int2string(Idx)), idx(Idx) { if (idx == 0xffffffffUL) idx = -1; }
	// symbol
	CScriptPropertyName(uint32_t Id, const std::string &Desc) : name(Desc), idx(-1-(int64_t)Id) {}
	bool operator<(const CScriptPropertyName& rhs) {
		int64_t lhs_idx = idx + 1;
		int64_t rhs_idx = rhs.idx + 1;
		if (lhs_idx < rhs_idx) return true;
		if (lhs_idx == 0 && rhs_idx == 0) return name < rhs.name;
		return false;
	}
	bool isArrayIdx() const { return 0 <= idx && idx < 0xFFFFFFFFLL; }
	bool isPropertyName() const { return idx == -1; }
	bool isSymbol() const { return idx < -1; }
private:
	static int64_t name2arrayIdx(const std::string &Name);
	std::string name;
	int64_t idx; /* -1 ==> normal Name; >=0 ==> arrayIdx; <-1 ==> Symbol*/
};

//////////////////////////////////////////////////////////////////////////
/// CScriptVar
//////////////////////////////////////////////////////////////////////////

typedef	std::vector<class CScriptVarLinkPtr> SCRIPTVAR_CHILDS_t;
typedef	SCRIPTVAR_CHILDS_t::iterator SCRIPTVAR_CHILDS_it;
typedef	SCRIPTVAR_CHILDS_t::reverse_iterator SCRIPTVAR_CHILDS_rit;
typedef	SCRIPTVAR_CHILDS_t::const_iterator SCRIPTVAR_CHILDS_cit;

// CScriptVar is the base class of all variable values.
// Instances of CScriptVar can only exists as pointer. CScriptVarPtr holds this pointer

// CScriptVar is the base class of all variable values.
//
class CScriptVar : public fixed_size_object<CScriptVar> {
protected:
	CScriptVar(CTinyJS* Context, const CScriptVarPtr& Prototype); ///< Create
	CScriptVar(const CScriptVar& Copy) MEMBER_DELETE; ///< Copy protected
private:
	CScriptVar & operator=(const CScriptVar &Copy) MEMBER_DELETE; ///< private -> no assignment-Copy
public:
	virtual ~CScriptVar();

	//************************************
	// Method:    getPrototype
	// FullName:  CScriptVar::getPrototype
	// Access:    public 
	// Returns:   CScriptVarPtr
	// Qualifier:
	//************************************
	CScriptVarPtr getPrototype();

	void setPrototype(const CScriptVarPtr& Prototype);

	/// Type
	virtual bool isObject();	///< is an Object
	virtual bool isArray();		///< is an Array
	virtual bool isDate();		///< is a Date-Object
	virtual bool isError();		///< is an ErrorObject
	virtual bool isRegExp();	///< is a RegExpObject
	virtual bool isAccessor();	///< is an Accessor
	virtual bool isNull();		///< is Null
	virtual bool isUndefined();	///< is Undefined
	bool isNullOrUndefined();	///< is Null or Undefined
	virtual bool isNaN();		///< is NaN
	virtual bool isString();	///< is String
	virtual bool isInt();		///< is Integer
	virtual bool isBool();		///< is Bool
	virtual int isInfinity();	///< is Infinity ///< +1==POSITIVE_INFINITY, -1==NEGATIVE_INFINITY, 0==is not an InfinityVar
	virtual bool isDouble();	///< is Double

	virtual bool isRealNumber();///< is isInt | isDouble
	virtual bool isNumber();	///< is isNaN | isInt | isDouble | isInfinity
	virtual bool isPrimitive();	///< isNull | isUndefined | isNaN | isString | isInt | isDouble | isInfinity

	virtual bool isFunction();	///< is CScriptVarFunction / CScriptVarFunctionNativeCallback / CScriptVarFunctionNativeClass
	virtual bool isNative();	///< is CScriptVarFunctionNativeCallback / CScriptVarFunctionNativeClass
	virtual bool isBounded();	///< is CScriptVarFunctionBounded

	virtual bool isIterator();
	virtual bool isGenerator();

	//bool isBasic() const { return Childs.empty(); } ///< Is this *not* an array/object/etc


	//////////////////////////////////////////////////////////////////////////
	/// Value
	//////////////////////////////////////////////////////////////////////////

	virtual CScriptVarPrimitivePtr getRawPrimitive()=0; ///< is Var==Primitive -> return this isObject return Value
	CScriptVarPrimitivePtr toPrimitive(); ///< by default call getDefaultValue_hintNumber by a Date-object calls getDefaultValue_hintString
	virtual CScriptVarPrimitivePtr toPrimitive(CScriptResult &execute); ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
	CScriptVarPrimitivePtr toPrimitive_hintString(int32_t radix=0); ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
	CScriptVarPrimitivePtr toPrimitive_hintString(CScriptResult &execute, int32_t radix=0); ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
	CScriptVarPrimitivePtr toPrimitive_hintNumber(); ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
	CScriptVarPrimitivePtr toPrimitive_hintNumber(CScriptResult &execute); ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself

	CScriptVarPtr callJS_toString(CScriptResult &execute, int radix=0);
	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0);
	CScriptVarPtr callJS_valueOf(CScriptResult &execute);
	virtual CScriptVarPtr valueOf_CallBack();

	CNumber toNumber();
	CNumber toNumber(CScriptResult &execute);
	virtual bool toBoolean();
	std::string toString(int32_t radix=0); ///< shortcut for this->toPrimitive_hintString()->toCString();
	std::string toString(CScriptResult &execute, int32_t radix=0); ///< shortcut for this->toPrimitive_hintString(execute)->toCString();

	int DEPRECATED("getInt() is deprecated use toNumber().toInt32 instead") getInt();
	bool DEPRECATED("getBool() is deprecated use toBoolean() instead") getBool();
	double DEPRECATED("getDouble() is deprecated use toNumber().toDouble() instead") getDouble();
	std::string DEPRECATED("getString() is deprecated use toString() instead") getString();

	virtual void setter(CScriptResult &execute, const CScriptVarLinkPtr &link, const CScriptVarPtr &value);
	virtual CScriptVarLinkPtr &getter(CScriptResult &execute, CScriptVarLinkPtr &link);


	virtual CScriptTokenDataFnc *getFunctionData(); ///< { return 0; }

	virtual CScriptVarPtr toObject()=0;

	CScriptVarPtr toIterator(IteratorMode Mode=RETURN_ARRAY);
	CScriptVarPtr toIterator(CScriptResult &execute, IteratorMode Mode=RETURN_ARRAY);

//	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, bool &hasRecursion); ///< get Data as a parsable javascript string
#define getParsableStringRecursionsCheckBegin() do{		\
		if(uniqueID) { \
			if(uniqueID==getTemporaryMark()) { hasRecursion=true; return "recursion"; } \
			setTemporaryMark(uniqueID); \
		} \
	} while(0)
#define getParsableStringRecursionsCheckEnd() setTemporaryMark(0)


	std::string getParsableString(); ///< get Data as a parsable javascript string
	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion); ///< get Data as a parsable javascript string
	virtual std::string getVarType()=0;

	CScriptVarPtr DEPRECATED("getNumericVar() is deprecated use toNumber() instead") getNumericVar(); ///< returns an Integer, a Double, an Infinity or a NaN

	//////////////////////////////////////////////////////////////////////////
	/// Childs
	//////////////////////////////////////////////////////////////////////////


	CScriptVarPtr getOwnPropertyDescriptor(const std::string &Name);
	const char *defineProperty(const std::string &Name, CScriptVarPtr Attributes);

	/// flags
	void setExtensible(bool On=true)	{ extensible=On; }
	void preventExtensions()			{ extensible=false; }
	bool isExtensible() const			{ return extensible; }
	void seal();
	bool isSealed() const;
	void freeze();
	bool isFrozen() const;

	/// find
	CScriptVarLinkPtr findChild(const std::string &childName); ///< Tries to find a child with the given name, may return 0
	CScriptVarLinkWorkPtr DEPRECATED("findChildWithStringChars is deprecated use getOwnProperty instead") findChildWithStringChars(const std::string &childName);
	virtual CScriptVarLinkWorkPtr getOwnProperty(const std::string &childName); ///< Tries to find a child with the given name, may return 0 or a faked porperty
	CScriptVarLinkPtr findChildInPrototypeChain(const std::string &childName);
	CScriptVarLinkWorkPtr findChildWithPrototypeChain(const std::string &childName);
	CScriptVarLinkPtr findChildByPath(const std::string &path); ///< Tries to find a child with the given path (separated by dots)
	CScriptVarLinkPtr findChildOrCreate(const std::string &childName/*, int varFlags=SCRIPTVAR_UNDEFINED*/); ///< Tries to find a child with the given name, or will create it with the given flags
	CScriptVarLinkPtr findChildOrCreateByPath(const std::string &path); ///< Tries to find a child with the given path (separated by dots)
	virtual void keys(STRING_SET_t &Keys, bool OnlyEnumerable=true, uint32_t ID=0);
	/// add & remove
	CScriptVarLinkPtr addChild(const std::string &childName, const CScriptVarPtr &child, int linkFlags = SCRIPTVARLINK_DEFAULT);
	CScriptVarLinkPtr addChild(uint32_t childName, const CScriptVarPtr &child, int linkFlags = SCRIPTVARLINK_DEFAULT);// { return addChild(int2string(childName), child, linkFlags); }
	CScriptVarLinkPtr DEPRECATED("addChildNoDup is deprecated use addChildOrReplace instead!") addChildNoDup(const std::string &childName, const CScriptVarPtr &child, int linkFlags = SCRIPTVARLINK_DEFAULT);
	CScriptVarLinkPtr addChildOrReplace(const std::string &childName, const CScriptVarPtr &child, int linkFlags = SCRIPTVARLINK_DEFAULT); ///< add a child overwriting any with the same name
	CScriptVarLinkPtr addChildOrReplace(uint32_t childName, const CScriptVarPtr &child, int linkFlags = SCRIPTVARLINK_DEFAULT);// { return addChildOrReplace(int2string(childName), child, linkFlags); }
	bool removeLink(CScriptVarLinkPtr &link); ///< Remove a specific link (this is faster than finding via a child)
	bool removeChild(const std::string &childName); ///< Remove a specific child
	virtual void removeAllChildren();

	/// useful for native functions
	CScriptVarPtr getProperty(const std::string &name);
	CScriptVarPtr getProperty(uint32_t idx);
	void setProperty(const std::string &name, const CScriptVarPtr &value, bool ignoreReadOnly=false, bool ignoreNotExtensible=false);
	void setProperty(uint32_t idx, const CScriptVarPtr &value, bool ignoreReadOnly=false, bool ignoreNotExtensible=false);
	virtual uint32_t getLength(); ///< get the value of the "length"-property if it in the range of 0 to 32^2-1 otherwise returns 0


	/// ARRAY
	CScriptVarPtr DEPRECATED("getArrayIndex is deprecated use getProperty instead!") getArrayIndex(uint32_t idx); ///< The the value at an array index
	void DEPRECATED("setArrayIndex is deprecated use setProperty instead!") setArrayIndex(uint32_t idx, const CScriptVarPtr &value); ///< Set the value at an array index
	uint32_t DEPRECATED("getArrayLength is deprecated use getLength instead!") getArrayLength(); ///< If this is an array, return the number of items in it (else 0)

	//////////////////////////////////////////////////////////////////////////
	size_t getChildren() const { return Childs.size(); } ///< Get the number of children
	CTinyJS *getContext() { return context; }
	CScriptVarPtr mathsOp(const CScriptVarPtr &b, int op); ///< do a maths op with another script variable

	void trace(const std::string &name = ""); ///< Dump out the contents of this using trace
	void trace(std::string &indentStr, uint32_t uniqueID, const std::string &name = ""); ///< Dump out the contents of this using trace
	std::string getFlagsAsString(); ///< For debugging - just dump a string version of the flags
//	void getJSON(std::ostringstream &destination, const std::string linePrefix=""); ///< Write out all the JS code needed to recreate this script variable to the stream (as JSON)

	SCRIPTVAR_CHILDS_t Childs;

private:
	CScriptVar *ref(); ///< Add reference to this variable
	void unref(); ///< Remove a reference, and delete this variable if required
public:
	int getRefs() const; ///< Get the number of references to this script variable

	// cast template
	template<class T>
	operator T *(){ T *ret = dynamic_cast<T*>(this); ASSERT(ret!=0); return ret; }
	template<class T>
	T *get(){ T *ret = dynamic_cast<T*>(this); ASSERT(ret!=0); return ret; }

	
	/// newScriptVar
	template<typename T>	CScriptVarPtr newScriptVar(T t); /// { return ::newScriptVar(context, t); }
	template<typename T1, typename T2>	CScriptVarPtr newScriptVar(T1 t1, T2 t2); // { return ::newScriptVar(context, t); }
	template<typename T>	const CScriptVarPtr &constScriptVar(T t); // { return ::newScriptVar(context, t); }

	/// For memory management/garbage collection
	void setTemporaryMark(uint32_t ID); // defined as inline at end of this file { temporaryMark[context->getCurrentMarkSlot()] = ID; }
	virtual void cleanUp4Destroy();
	virtual void setTemporaryMark_recursive(uint32_t ID);
	uint32_t getTemporaryMark(); // defined as inline at end of this file { return temporaryMark[context->getCurrentMarkSlot()]; }
protected:
	bool extensible;
	CTinyJS *context;
	int refs; ///< The number of references held to this - used for garbage collection
	CScriptVar *prototype;
	CScriptVar *prev;
	CScriptVar *next;
	uint32_t temporaryMark[TEMPORARY_MARK_SLOTS];
	friend class CTinyJS;
	friend class CScriptVarPtr;
#if _DEBUG
	uint32_t debugID;
#endif
};


//////////////////////////////////////////////////////////////////////////
/// CScriptVarPtr
//////////////////////////////////////////////////////////////////////////

/// CScriptVarPtr is the Pointer Class for instances of CScriptVar aka JS values
class CScriptVarPtr {
public:
	// construct
	CScriptVarPtr() : var(0) {} ///< 0-Pointer
	CScriptVarPtr(CScriptVar *Var) : var(Var) { if(var) var->ref(); } // creates a new CScriptVar (from new);

	// copy
	CScriptVarPtr(const CScriptVarPtr &Copy) : var(Copy.var) { if(var) var->ref(); }
	CScriptVarPtr& operator=(const CScriptVarPtr &Copy) {
		if(var != Copy.var) { // if old_var == new_var do nothing
			if(var) var->unref();	// unref old_var
			var = Copy.var; if(var) var->ref(); // ref new_var
		}
		return *this;
	}
#if HAVE_CXX11_RVALUE_REFERENCE
	// move
	CScriptVarPtr(CScriptVarPtr &&Other) NOEXCEPT : var(Other.var) { Other.var=0; }
	CScriptVarPtr& operator=(CScriptVarPtr &&Other) NOEXCEPT { if(var) var->unref(); var = Other.var; Other.var=0; return *this; }
#endif
	// deconstruct
	~CScriptVarPtr() { if(var) var->unref(); }

	// if
	operator bool() const { return var!=0; }

	bool operator ==(const CScriptVarPtr &Other) const { return var == Other.var; }
	bool operator !=(const CScriptVarPtr &Other) const { return var != Other.var; }

	// access
	CScriptVar * operator ->() const { return var; }
	CScriptVar *getVar() const { return var; }

	void clear() { if(var) var->unref(); var=0; }
protected:
	CScriptVar *var;
};

//////////////////////////////////////////////////////////////////////////
/// CScriptVarPointer - template
//////////////////////////////////////////////////////////////////////////

template<typename C>
class CScriptVarPointer : public CScriptVarPtr {
public:
	CScriptVarPointer() {}
	CScriptVarPointer(CScriptVar *Var) : CScriptVarPtr(dynamic_cast<C*>(Var)) {}
	CScriptVarPointer(const CScriptVarPtr &Copy) : CScriptVarPtr(dynamic_cast<C*>(Copy.getVar())) {}
	CScriptVarPointer<C> &operator=(const CScriptVarPtr &Copy) { CScriptVarPtr::operator=(dynamic_cast<C*>(Copy.getVar())); return *this; }
	C * operator ->() const { C *Var = dynamic_cast<C*>(var); ASSERT(var && Var); return Var; }
};


//////////////////////////////////////////////////////////////////////////
/// CScriptVarLink
//////////////////////////////////////////////////////////////////////////

/*!
 *  CScriptVarLink is the real javascript variable. This class holds a pointer of CScriptVarLink
 *  1. a name
 *  2. a CScriptVarPtr as value
 *  3. an owner (scope or object)
 */
class CScriptVarLink : public fixed_size_object<CScriptVarLink>
{
private: // prevent gloabal creating
	/*!
	 *  Constructor.
	 *
	 *      @param [in] var   
	 *      @param [in] name  
	 *      @param [in] flags 
	 */
	CScriptVarLink(const CScriptVarPtr &var, const std::string &name = TINYJS_TEMP_NAME, int flags = SCRIPTVARLINK_DEFAULT);
private: // prevent Copy
	CScriptVarLink(const CScriptVarLink &link) MEMBER_DELETE; ///< Copy constructor
public:
	~CScriptVarLink();

	const std::string &getName() const { return name; }

	void reName(const std::string newName) { name = newName; } ///< !!!! Danger !!!! - need resort of childs-array

	int getFlags() const { return flags; }
	const CScriptVarPtr &getVarPtr() const { return var; }
	const CScriptVarPtr &setVarPtr(const CScriptVarPtr &Var) { return var = Var; } ///< simple Replace the Variable pointed to


	bool isOwned() const { return owner!=0; }

	bool isWritable() const { return (flags & SCRIPTVARLINK_WRITABLE) != 0; }
	void setWritable(bool On) { On ? (flags |= SCRIPTVARLINK_WRITABLE) : (flags &= ~SCRIPTVARLINK_WRITABLE); }
	bool isConfigurable() const { return (flags & SCRIPTVARLINK_CONFIGURABLE) != 0; }
	void setConfigurable(bool On) { On ? (flags |= SCRIPTVARLINK_CONFIGURABLE) : (flags &= ~SCRIPTVARLINK_CONFIGURABLE); }
	bool isEnumerable() const { return (flags & SCRIPTVARLINK_ENUMERABLE) != 0; }
	void setEnumerable(bool On) { On ? (flags |= SCRIPTVARLINK_ENUMERABLE) : (flags &= ~SCRIPTVARLINK_ENUMERABLE); }

	CScriptVar *getOwner() { return owner; };
	void setOwner(CScriptVar *Owner) { owner = Owner; }

	/// forward to ScriptVar

	CScriptVarPrimitivePtr toPrimitive() { ///< by default call getDefaultValue_hintNumber by a Date-object calls getDefaultValue_hintString
		return var->toPrimitive(); }
	CScriptVarPrimitivePtr toPrimitive(CScriptResult &execute) { ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
		return var->toPrimitive(execute); }
	CScriptVarPrimitivePtr toPrimitive_hintString(int32_t radix=0) { ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
		return var->toPrimitive_hintString(radix); }
	CScriptVarPrimitivePtr toPrimitive_hintString(CScriptResult &execute, int32_t radix=0) { ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
		return var->toPrimitive_hintString(execute, radix); }
	CScriptVarPrimitivePtr toPrimitive_hintNumber() { ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
		return var->toPrimitive_hintNumber(); }
	CScriptVarPrimitivePtr toPrimitive_hintNumber(CScriptResult &execute) { ///< if the var an ObjectType gets the valueOf; if valueOf of an ObjectType gets toString / otherwise gets the Var itself
		return var->toPrimitive_hintNumber(execute); }

	CNumber toNumber(); // { return var->toNumber(); }
	CNumber toNumber(CScriptResult &execute); // { return var->toNumber(execute); }
	bool toBoolean() { return var->toBoolean(); }
	std::string toString(int32_t radix=0) { ///< shortcut for this->toPrimitive_hintString()->toCString();
		return var->toString(radix); }
	std::string toString(CScriptResult &execute, int32_t radix=0) { ///< shortcut for this->toPrimitive_hintString(execute)->toCString();
		return var->toString(execute, radix); }
	CScriptVarPtr toObject() { return var->toObject(); };

private:
	std::string name;
	CScriptVar *owner; // pointer to the owner CScriptVar
	uint32_t flags;
	CScriptVarPtr var;
#ifdef _DEBUG
	char dummy[24];
#endif
	CScriptVarLink *ref();
	void unref();
private:
	int refs;
	friend class CScriptVarLinkPtr;
};


//////////////////////////////////////////////////////////////////////////
/// CScriptVarLinkPtr
//////////////////////////////////////////////////////////////////////////


/*!
 *  CScriptVarLinkPtr holds a pointer of CScriptVarLink
 */
class CScriptVarLinkPtr {
public:
	// construct
	CScriptVarLinkPtr() : link(0) {} ///< 0-Pointer

	CScriptVarLinkPtr(const CScriptVarPtr& var, const std::string& name = TINYJS_TEMP_NAME, int flags = SCRIPTVARLINK_DEFAULT) { link = (new CScriptVarLink(var, name, flags))->ref(); }
	CScriptVarLinkPtr(CScriptVarLink *Link) : link(Link) { if(link) link->ref(); } // creates a new CScriptVarLink (from new);

	// reconstruct

	CScriptVarLinkPtr& operator()(const CScriptVarPtr& var, const std::string& name = TINYJS_TEMP_NAME, int flags = SCRIPTVARLINK_DEFAULT);
	CScriptVarLinkPtr &operator=(const CScriptVarPtr &var) { return operator()(var); }
	// deconstruct
	~CScriptVarLinkPtr() { if(link) link->unref(); }

	// copy
	CScriptVarLinkPtr(const CScriptVarLinkPtr &Copy) : link(Copy.link) { if(link) link->ref(); }
	CScriptVarLinkPtr &operator=(const CScriptVarLinkPtr &Copy) {
		if(link != Copy.link) {
			if(link) link->unref();
			link = Copy.link; if(link) link->ref();
		}
		return *this;
	}
	
#if HAVE_CXX11_RVALUE_REFERENCE
	// move
	CScriptVarLinkPtr(CScriptVarLinkPtr &&Other) NOEXCEPT : link(Other.link) { Other.link = 0; }
	CScriptVarLinkPtr &operator=(CScriptVarLinkPtr &&Other) NOEXCEPT { if(link) link->unref(); link = Other.link; Other.link = 0; return *this; }
#endif

	CScriptVarLinkWorkPtr getter();
	CScriptVarLinkWorkPtr getter(CScriptResult &execute);
	CScriptVarLinkWorkPtr setter(const CScriptVarPtr &Var);
	CScriptVarLinkWorkPtr setter(CScriptResult &execute, const CScriptVarPtr &Var);

	/*!
	 *  Operators the bool.
	 *
	 *      @return 
	 */
	operator bool() const { return link!=0; }

	// for sorting in child-list
	bool operator<(const std::string &rhs) const;
	bool operator ==(const CScriptVarLinkPtr &rhs) const { return link==rhs.link; }
	// access to CScriptVarLink
	CScriptVarLink *operator ->() const { return link; }

	operator const CScriptVarPtr &() const { static CScriptVarPtr NullPtr; return link?link->getVarPtr():NullPtr; }

	void clear() { if(link) link->unref(); link=0; }
protected:
	CScriptVarLink *link;
};


//////////////////////////////////////////////////////////////////////////
/// CScriptVarLinkWorkPtr
//////////////////////////////////////////////////////////////////////////

/*!
 *  CScriptVarLinkWorkPtr is a special variant of CScriptVarLinkPtr
 *  The CScriptVarLinkWorkPtr have in addition a referencedOwner
 *  referencedOwner is useful 
 */
class CScriptVarLinkWorkPtr : public CScriptVarLinkPtr {
public:
	// construct
	CScriptVarLinkWorkPtr() {}
	CScriptVarLinkWorkPtr(const CScriptVarPtr &var, const std::string &name = TINYJS_TEMP_NAME, int flags = SCRIPTVARLINK_DEFAULT) : CScriptVarLinkPtr(var, name, flags) {}
	CScriptVarLinkWorkPtr(CScriptVarLink *Link) : CScriptVarLinkPtr(Link) { if(link) referencedOwner = link->getOwner(); } // creates a new CScriptVarLink (from new);
	CScriptVarLinkWorkPtr(const CScriptVarLinkPtr &Copy) : CScriptVarLinkPtr(Copy) { if(link) referencedOwner = link->getOwner(); }

	// reconstruct
	CScriptVarLinkWorkPtr &operator()(const CScriptVarPtr &var, const std::string &name = TINYJS_TEMP_NAME, int flags = SCRIPTVARLINK_DEFAULT) {CScriptVarLinkPtr::operator()(var, name, flags); referencedOwner.clear(); return *this; }

	// copy
	CScriptVarLinkWorkPtr(const CScriptVarLinkWorkPtr &Copy) : CScriptVarLinkPtr(Copy), referencedOwner(Copy.referencedOwner) {}
	CScriptVarLinkWorkPtr &operator=(const CScriptVarLinkWorkPtr &Copy) { CScriptVarLinkPtr::operator=(Copy); referencedOwner = Copy.referencedOwner; return *this; }

#if HAVE_CXX11_RVALUE_REFERENCE
	// move
	CScriptVarLinkWorkPtr(CScriptVarLinkWorkPtr &&Other) NOEXCEPT : CScriptVarLinkPtr(std::move(Other)), referencedOwner(std::move(Other.referencedOwner)) {}
	CScriptVarLinkWorkPtr &operator=(CScriptVarLinkWorkPtr &&Other) NOEXCEPT { CScriptVarLinkPtr::operator=(std::move(Other)); referencedOwner = std::move(Other.referencedOwner); return *this; }
#endif
	// assign
	//void assign(CScriptVarPtr rhs, bool ignoreReadOnly=false, bool ignoreNotOwned=false, bool ignoreNotExtensible=false);



	CScriptVarLinkWorkPtr getter();
	CScriptVarLinkWorkPtr getter(CScriptResult &execute);
	CScriptVarLinkWorkPtr setter(const CScriptVarPtr &Var);
	CScriptVarLinkWorkPtr setter(CScriptResult &execute, const CScriptVarPtr &Var);


	void swap(CScriptVarLinkWorkPtr &Link) {
		CScriptVarPtr _referencedOwner = referencedOwner; referencedOwner = Link.referencedOwner; Link.referencedOwner = _referencedOwner;
		CScriptVarLink *_link=link; link=Link.link; Link.link=_link;
	}

	void clear() { CScriptVarLinkPtr::clear(); referencedOwner.clear(); }
	void setReferencedOwner(const CScriptVarPtr &Owner) { referencedOwner = Owner; }
	const CScriptVarPtr &getReferencedOwner() const { return referencedOwner; }
	bool hasReferencedOwner() const { return referencedOwner; }
private:
	CScriptVarPtr referencedOwner;
};

inline CScriptVarLinkWorkPtr CScriptVarLinkPtr::getter() { return CScriptVarLinkWorkPtr(*this).getter(); }
inline CScriptVarLinkWorkPtr CScriptVarLinkPtr::getter( CScriptResult &execute ) { return CScriptVarLinkWorkPtr(*this).getter(execute); }
inline CScriptVarLinkWorkPtr CScriptVarLinkPtr::setter( const CScriptVarPtr &Var ) { return CScriptVarLinkWorkPtr(*this).setter(Var); }
inline CScriptVarLinkWorkPtr CScriptVarLinkPtr::setter( CScriptResult &execute, const CScriptVarPtr &Var ) { return CScriptVarLinkWorkPtr(*this).setter(execute, Var); }

//////////////////////////////////////////////////////////////////////////
#define define_dummy_t(t1) struct t1##_t{}; extern t1##_t t1
#define declare_dummy_t(t1) t1##_t t1
#define define_newScriptVar_Fnc(t1, ...) CScriptVarPtr newScriptVar(__VA_ARGS__)
#define define_newScriptVar_NamedFnc(t1, ...) CScriptVarPtr newScriptVar##t1(__VA_ARGS__)
#define define_ScriptVarPtr_Type(t1) class CScriptVar##t1; typedef CScriptVarPointer<CScriptVar##t1> CScriptVar##t1##Ptr

#define define_DEPRECATED_newScriptVar_Fnc(t1, ...) CScriptVarPtr DEPRECATED("newScriptVar("#__VA_ARGS__") is deprecated use constScriptVar("#__VA_ARGS__") instead") newScriptVar(__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////
/// CScriptVarPrimitive
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(Primitive);
class CScriptVarPrimitive : public CScriptVar {
protected:
	CScriptVarPrimitive(CTinyJS *Context, const CScriptVarPtr &Prototype) : CScriptVar(Context, Prototype) { setExtensible(false); }
	CScriptVarPrimitive(const CScriptVarPrimitive& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarPrimitive() OVERRIDE;

	virtual bool isPrimitive() OVERRIDE;	///< return true;

	virtual CScriptVarPrimitivePtr getRawPrimitive() OVERRIDE;
	virtual bool toBoolean() OVERRIDE;							/// false by default
	virtual CNumber toNumber_Callback()=0;
	virtual std::string toCString(int radix=0)=0;

	virtual CScriptVarPtr toObject() OVERRIDE;
	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;
protected:
};


//////////////////////////////////////////////////////////////////////////
/// CScriptVarUndefined
//////////////////////////////////////////////////////////////////////////

define_dummy_t(Undefined);
define_ScriptVarPtr_Type(Undefined);
class CScriptVarUndefined : public CScriptVarPrimitive {
protected:
	CScriptVarUndefined(CTinyJS *Context);
	CScriptVarUndefined(const CScriptVarUndefined& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarUndefined() OVERRIDE;

	virtual bool isUndefined() OVERRIDE; // { return true; }

	virtual CNumber toNumber_Callback() OVERRIDE; // { return NaN; }
	virtual std::string toCString(int radix=0) OVERRIDE;// { return "undefined"; }

	virtual std::string getVarType() OVERRIDE; // { return "undefined"; }
	friend define_DEPRECATED_newScriptVar_Fnc(Undefined, CTinyJS *, Undefined_t);
	friend define_newScriptVar_NamedFnc(Undefined, CTinyJS *Context);
};
inline define_DEPRECATED_newScriptVar_Fnc(Undefined, CTinyJS *Context, Undefined_t) { return new CScriptVarUndefined(Context); }
inline define_newScriptVar_NamedFnc(Undefined, CTinyJS *Context) { return new CScriptVarUndefined(Context); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarNull
//////////////////////////////////////////////////////////////////////////

define_dummy_t(Null);
define_ScriptVarPtr_Type(Null);
class CScriptVarNull : public CScriptVarPrimitive {
protected:
	CScriptVarNull(CTinyJS *Context);
	CScriptVarNull(const CScriptVarNull& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarNull() OVERRIDE;

	virtual bool isNull() OVERRIDE; // { return true; }

	virtual CNumber toNumber_Callback() OVERRIDE; // { return 0; }
	virtual std::string toCString(int radix=0) OVERRIDE;// { return "null"; }

	virtual std::string getVarType() OVERRIDE; // { return "null"; }

	friend define_DEPRECATED_newScriptVar_Fnc(Null, CTinyJS *Context, Null_t);
	friend define_newScriptVar_NamedFnc(Null, CTinyJS *Context);
};
inline define_DEPRECATED_newScriptVar_Fnc(Null, CTinyJS *Context, Null_t) { return new CScriptVarNull(Context); }
inline define_newScriptVar_NamedFnc(Null, CTinyJS *Context) { return new CScriptVarNull(Context); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarString
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(String);
class CScriptVarString : public CScriptVarPrimitive {
protected:
	CScriptVarString(CTinyJS *Context, const std::string &Data);
	CScriptVarString(const CScriptVarString& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarString() OVERRIDE;
	virtual bool isString() OVERRIDE; // { return true; }

	virtual bool toBoolean() OVERRIDE;
	virtual CNumber toNumber_Callback() OVERRIDE;
	virtual std::string toCString(int radix=0) OVERRIDE;

	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) OVERRIDE; // { return getJSString(data); }
	virtual std::string getVarType() OVERRIDE; // { return "string"; }

	virtual CScriptVarPtr toObject() OVERRIDE;
	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;

	virtual CScriptVarLinkWorkPtr getOwnProperty(const std::string &childName) OVERRIDE;
	virtual void keys(STRING_SET_t &Keys, bool OnlyEnumerable=true, uint32_t ID=0) OVERRIDE;


	size_t DEPRECATED("stringLength is deprecated use getLength instead!") stringLength() { return data.size(); }
	virtual uint32_t getLength() OVERRIDE;
	int getChar(uint32_t Idx);
protected:
	std::string data;
private:
	friend define_newScriptVar_Fnc(String, CTinyJS *Context, const std::string &);
	friend define_newScriptVar_Fnc(String, CTinyJS *Context, const char *);
	friend define_newScriptVar_Fnc(String, CTinyJS *Context, char *);
};
inline define_newScriptVar_Fnc(String, CTinyJS *Context, const std::string &Obj) { return new CScriptVarString(Context, Obj); }
inline define_newScriptVar_Fnc(String, CTinyJS *Context, const char *Obj) { return new CScriptVarString(Context, Obj); }
inline define_newScriptVar_Fnc(String, CTinyJS *Context, char *Obj) { return new CScriptVarString(Context, Obj); }


//////////////////////////////////////////////////////////////////////////
/// CNumber
//////////////////////////////////////////////////////////////////////////
define_dummy_t(NegativeZero);
define_dummy_t(NaN);
class Infinity { public:Infinity(int Sig = 1) :sig(Sig) {} int Sig() const { return sig; } private:int sig; };
extern Infinity InfinityPositive;
extern Infinity InfinityNegative;

class CNumber {
private:
	enum NType {
		tnNULL, tInt32, tDouble, tNaN, tInfinity
	};
	CNumber(NType Type, int32_t InfinitySign=0) : type(Type) { Int32 = InfinitySign; }
public:

	CNumber(const CNumber &Copy) { *this=Copy; }

	CNumber(int32_t Value=0) : type(tInt32) { Int32=Value; }
#if 1
	template<typename T>CNumber(T Value) { *this = Value; }
#else
	CNumber(negativeZero_t Value) { *this = Value; }
	CNumber(NaN_t Value)  { *this = Value; }
	CNumber(Infinity Value) { *this = Value; }
	CNumber(uint32_t Value) { *this = Value; }
	CNumber(double Value) { *this = Value; }
	CNumber(unsigned char Value) { *this = Value; }
	CNumber(const char *Value) { *this = Value; }
	CNumber(const std::string &Value) { *this = Value; }
#endif
	CNumber &operator=(NegativeZero_t) { type=tnNULL; Int32=0; return *this; }
	CNumber &operator=(NaN_t) { type=tNaN; Int32=0; return *this; }
	CNumber &operator=(Infinity v) { type=tInfinity; Int32=v.Sig(); return *this; }
	CNumber &operator=(int32_t Value) { type=tInt32; Int32=Value; return *this; }
	CNumber &operator=(uint32_t Value) {
		if(Value<=0x7ffffffUL)
			type=tInt32, Int32=int32_t(Value);
		else
			type=tDouble, Double=Value;
		return *this;
	}
	CNumber &operator=(uint64_t Value);
	CNumber &operator=(int64_t Value);
	CNumber &operator=(double Value);
	CNumber &operator=(unsigned char Value) { type=tInt32; Int32=Value; return *this; }
	CNumber &operator=(const char *Value);
	CNumber &operator=(const std::string &Value) { return operator=(Value.c_str());}

	int32_t parseInt(const char *str, int32_t radix=0, const char **endptr=0);
	void parseInt(const std::string &str, int32_t radix=0) { parseInt(str.c_str(), radix); }
	void parseFloat(const char *str, const char **endptr=0);
	void parseFloat(const std::string &str) { parseFloat(str.c_str()); }

	CNumber add(const CNumber &Value) const;
	CNumber operator-() const;
	CNumber operator~() const { if(type==tNaN) return *this; else return ~toInt32(); }
	bool operator!() const { return isZero(); }
	CNumber multi(const CNumber &Value) const;
	CNumber div(const CNumber& Value) const;
	CNumber pow(const CNumber& Value) const;
	CNumber modulo(const CNumber &Value) const;

	CNumber round() const;
	CNumber floor() const;
	CNumber ceil() const;
	CNumber abs() const;

	CNumber shift(const CNumber &Value, bool right) const;
	CNumber ushift(const CNumber &Value, bool right=true) const;

	CNumber binary(const CNumber &Value, char Mode) const;
	CNumber clamp(const CNumber &min, const CNumber &max) const;

	int less(const CNumber &Value) const;
	bool equal(const CNumber &Value) const;


	bool isInt32() const { return type == tInt32; }
	bool isUInt32() const { return (type == tInt32 && Int32 >= 0) || (type == tDouble && Double <= std::numeric_limits<uint32_t>::max()) || (type == tnNULL); }
	bool isDouble() const { return type == tDouble; }

	bool isNaN() const { return type == tNaN; }
	int isInfinity() const { return type == tInfinity ? Int32 : 0; }
	bool isFinite() const { return type != tInfinity && type != tNaN; }
	bool isNegativeZero() const { return type==tnNULL; }
	bool isZero() const; ///< is 0, -0
	bool isInteger() const;
	int sign() const;

	int32_t		toInt32() const { return cast<int32_t>(); }
	uint32_t	toUInt32() const { return cast<uint32_t>(); }
	double		toDouble() const;
	bool		toBoolean() const { return !isZero() && type!=tNaN; }
	std::string	toString(uint32_t Radix=10) const;
private:
	template<typename T> T cast() const {
		switch(type) {
		case tInt32:
			return T(Int32);
		case tDouble:
			return T(Double);
		default:
			return T(0);
		}
	}
	NType type;
	union {
		int32_t	Int32;
		double	Double;
	};
};
inline CNumber operator+(const CNumber &lhs, const CNumber &rhs) { return lhs.add(rhs); }
inline CNumber &operator+=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.add(rhs); }
inline CNumber operator-(const CNumber &lhs, const CNumber &rhs) { return lhs.add(-rhs); }
inline CNumber &operator-=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.add(-rhs); }
inline CNumber operator*(const CNumber &lhs, const CNumber &rhs) { return lhs.multi(rhs); }
inline CNumber &operator*=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.multi(rhs); }
inline CNumber operator/(const CNumber &lhs, const CNumber &rhs) { return lhs.div(rhs); }
inline CNumber &operator/=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.div(rhs); }
inline CNumber operator%(const CNumber &lhs, const CNumber &rhs) { return lhs.modulo(rhs); }
inline CNumber &operator%=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.modulo(rhs); }
inline CNumber operator>>(const CNumber &lhs, const CNumber &rhs) { return lhs.shift(rhs, true); }
inline CNumber &operator>>=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.shift(rhs, true); }
inline CNumber operator<<(const CNumber &lhs, const CNumber &rhs) { return lhs.shift(rhs, false); }
inline CNumber &operator<<=(CNumber &lhs, const CNumber &rhs) { return lhs=lhs.shift(rhs, false); }

inline bool operator==(const CNumber &lhs, const CNumber &rhs) { return lhs.equal(rhs); }
inline bool operator!=(const CNumber &lhs, const CNumber &rhs) { return !lhs.equal(rhs); }
inline bool operator<(const CNumber &lhs, const CNumber &rhs) { return lhs.less(rhs)>0; }
inline bool operator<=(const CNumber &lhs, const CNumber &rhs) { return rhs.less(lhs)<0; }
inline bool operator>(const CNumber &lhs, const CNumber &rhs) { return rhs.less(lhs)>0; }
inline bool operator>=(const CNumber &lhs, const CNumber &rhs) { return lhs.less(rhs)<0; }

inline CNumber round(const CNumber &lhs) { return lhs.round(); }
inline CNumber floor(const CNumber &lhs) { return lhs.floor(); }
inline CNumber ceil(const CNumber &lhs) { return lhs.ceil(); }
inline CNumber abs(const CNumber &lhs) { return lhs.abs(); }

//////////////////////////////////////////////////////////////////////////
/// CScriptVarNumber
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(Number);
class CScriptVarNumber : public CScriptVarPrimitive {
protected:
	CScriptVarNumber(CTinyJS *Context, const CNumber &Data);
	CScriptVarNumber(const CScriptVarNumber& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarNumber() OVERRIDE;
	virtual bool isNumber() OVERRIDE; // { return true; }
	virtual bool isInt() OVERRIDE; // { return true; }
	virtual bool isDouble() OVERRIDE; // { return true; }
	virtual bool isRealNumber() OVERRIDE; // { return true; }
	virtual int isInfinity() OVERRIDE; // { return data; }
	virtual bool isNaN() OVERRIDE;// { return true; }

	virtual bool toBoolean() OVERRIDE;
	virtual CNumber toNumber_Callback() OVERRIDE;
	virtual std::string toCString(int radix=0) OVERRIDE;

	virtual std::string getVarType() OVERRIDE; // { return "number"; }

	virtual CScriptVarPtr toObject() OVERRIDE;
private:
	CNumber data;
	friend define_newScriptVar_Fnc(Number, CTinyJS *Context, const CNumber &);
	friend define_newScriptVar_NamedFnc(Number, CTinyJS *Context, const CNumber &);
};
define_newScriptVar_Fnc(Number, CTinyJS *Context, const CNumber &Obj);
inline define_newScriptVar_NamedFnc(Number, CTinyJS *Context, const CNumber &Obj) { return new CScriptVarNumber(Context, Obj); }
inline define_newScriptVar_Fnc(Number, CTinyJS *Context, char Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }
inline define_newScriptVar_Fnc(Number, CTinyJS *Context, int32_t Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }
inline define_newScriptVar_Fnc(Number, CTinyJS *Context, uint32_t Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }
inline define_newScriptVar_Fnc(Number, CTinyJS *Context, int64_t Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }
inline define_newScriptVar_Fnc(Number, CTinyJS *Context, uint64_t Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }
inline define_newScriptVar_Fnc(Number, CTinyJS *Context, double Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }
inline define_DEPRECATED_newScriptVar_Fnc(NaN, CTinyJS *Context, NaN_t) { return newScriptVarNumber(Context, CNumber(NaN)); }
inline define_DEPRECATED_newScriptVar_Fnc(Infinity, CTinyJS *Context, Infinity Obj) { return newScriptVarNumber(Context, CNumber(Obj)); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarBool
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(Bool);
class CScriptVarBool : public CScriptVarPrimitive {
protected:
	CScriptVarBool(CTinyJS *Context, bool Data);
	CScriptVarBool(const CScriptVarBool& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarBool() OVERRIDE;
	virtual bool isBool() OVERRIDE; // { return true; }

	virtual bool toBoolean() OVERRIDE;
	virtual CNumber toNumber_Callback() OVERRIDE;
	virtual std::string toCString(int radix=0) OVERRIDE;

	virtual std::string getVarType() OVERRIDE; // { return "boolean"; }

	virtual CScriptVarPtr toObject() OVERRIDE;
protected:
	bool data;

	friend define_DEPRECATED_newScriptVar_Fnc(Bool, CTinyJS *, bool);
	friend define_newScriptVar_NamedFnc(Bool, CTinyJS *Context, bool);
};
inline define_DEPRECATED_newScriptVar_Fnc(Bool, CTinyJS *Context, bool Obj) { return new CScriptVarBool(Context, Obj); }
inline define_newScriptVar_NamedFnc(Bool, CTinyJS *Context, bool Obj) { return new CScriptVarBool(Context, Obj); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarObject
//////////////////////////////////////////////////////////////////////////

define_dummy_t(Object);
define_ScriptVarPtr_Type(Object);

class CScriptVarObject : public CScriptVar {
protected:
	CScriptVarObject(CTinyJS *Context);
	CScriptVarObject(CTinyJS *Context, const CScriptVarPtr &Prototype) : CScriptVar(Context, Prototype) {}
	CScriptVarObject(CTinyJS *Context, const CScriptVarPrimitivePtr &Value, const CScriptVarPtr &Prototype) : CScriptVar(Context, Prototype), value(Value) {}
	CScriptVarObject(const CScriptVarObject& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarObject() OVERRIDE;

	virtual void removeAllChildren() OVERRIDE;

	virtual CScriptVarPrimitivePtr getRawPrimitive() OVERRIDE;
	virtual bool isObject() OVERRIDE; // { return true; }

	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) OVERRIDE;
	virtual std::string getVarType() OVERRIDE; ///< always "object"
	virtual std::string getVarTypeTagName(); ///< always "Object"
	virtual CScriptVarPtr toObject() OVERRIDE;

	virtual CScriptVarPtr valueOf_CallBack() OVERRIDE;
	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;
	virtual void setTemporaryMark_recursive(uint32_t ID) OVERRIDE;
protected:
private:
	CScriptVarPrimitivePtr value;
	friend define_newScriptVar_Fnc(Object, CTinyJS *Context, Object_t);
	friend define_newScriptVar_Fnc(Object, CTinyJS *Context, Object_t, const CScriptVarPtr &);
	friend define_newScriptVar_Fnc(Object, CTinyJS *Context, const CScriptVarPtr &);
	friend define_newScriptVar_Fnc(Object, CTinyJS *Context, const CScriptVarPrimitivePtr &, const CScriptVarPtr &);
};
inline define_newScriptVar_Fnc(Object, CTinyJS *Context, Object_t) { return new CScriptVarObject(Context); }
inline define_newScriptVar_Fnc(Object, CTinyJS *Context, Object_t, const CScriptVarPtr &Prototype) { return new CScriptVarObject(Context, Prototype); }
inline define_newScriptVar_Fnc(Object, CTinyJS *Context, const CScriptVarPtr &Prototype) { return new CScriptVarObject(Context, Prototype); }
inline define_newScriptVar_Fnc(Object, CTinyJS *Context, const CScriptVarPrimitivePtr &Value, const CScriptVarPtr &Prototype) { return new CScriptVarObject(Context, Value, Prototype); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarObjectTyped (simple Object with TypeTagName)
//////////////////////////////////////////////////////////////////////////

define_dummy_t(StopIteration);

class CScriptVarObjectTypeTagged : public CScriptVarObject {
protected:
//	CScriptVarObjectTyped(CTinyJS *Context);
	CScriptVarObjectTypeTagged(CTinyJS *Context, const CScriptVarPtr &Prototype, const std::string &TypeTagName) : CScriptVarObject(Context, Prototype), typeTagName(TypeTagName) {}
	CScriptVarObjectTypeTagged(const CScriptVarObjectTypeTagged& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarObjectTypeTagged() OVERRIDE;

	virtual std::string getVarTypeTagName() OVERRIDE;
protected:
private:
	std::string typeTagName;
	friend define_newScriptVar_Fnc(Object, CTinyJS *Context, Object_t, const CScriptVarPtr &, const std::string &);
	friend define_newScriptVar_Fnc(Object, CTinyJS *Context, const CScriptVarPtr &, const std::string &);
};
inline define_newScriptVar_Fnc(Object, CTinyJS *Context, Object_t, const CScriptVarPtr &Prototype, const std::string &TypeTagName) { return new CScriptVarObjectTypeTagged(Context, Prototype, TypeTagName); }
inline define_newScriptVar_Fnc(Object, CTinyJS *Context, const CScriptVarPtr &Prototype, const std::string &TypeTagName) { return new CScriptVarObjectTypeTagged(Context, Prototype, TypeTagName); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarError
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(Error);

class CScriptVarError : public CScriptVarObject {
protected:
	CScriptVarError(CTinyJS *Context, ERROR_TYPES type, const char *message, const char *file, int line, int column);// : CScriptVarObject(Context), value(Value) {}
	CScriptVarError(const CScriptVarError& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarError() OVERRIDE;
	virtual bool isError() OVERRIDE; // { return true; }

//	virtual std::string getParsableString(const std::string &indentString, const std::string &indent) OVERRIDE; ///< get Data as a parsable javascript string

	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;
	CScriptException toCScriptException();
private:
	friend define_newScriptVar_NamedFnc(Error, CTinyJS *Context, ERROR_TYPES type, const char *message, const char *file, int line, int column);
	friend define_newScriptVar_NamedFnc(Error, CTinyJS *Context, const CScriptException &Exception);
};
inline define_newScriptVar_NamedFnc(Error, CTinyJS *Context, ERROR_TYPES type, const char *message=0, const char *file=0, int line=-1, int column=-1) { return new CScriptVarError(Context, type, message, file, line, column); }
inline define_newScriptVar_NamedFnc(Error, CTinyJS *Context, const CScriptException &Exception) { return new CScriptVarError(Context, Exception.errorType, Exception.message.c_str(), Exception.fileName.c_str(), Exception.lineNumber, Exception.column); }

//////////////////////////////////////////////////////////////////////////
/// CScriptVarArray
//////////////////////////////////////////////////////////////////////////

define_dummy_t(Array);
define_ScriptVarPtr_Type(Array);
class CScriptVarArray : public CScriptVarObject {
protected:
	CScriptVarArray(CTinyJS *Context);
	CScriptVarArray(const CScriptVarArray& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarArray() OVERRIDE;
	virtual bool isArray() OVERRIDE; // { return true; }

	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) OVERRIDE;

	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;
	virtual void setter(CScriptResult &execute, const CScriptVarLinkPtr &link, const CScriptVarPtr &value) OVERRIDE;
	virtual CScriptVarLinkPtr &getter(CScriptResult &execute, CScriptVarLinkPtr &link) OVERRIDE;


	uint32_t getLength();
	CScriptVarPtr getArrayElement(uint32_t idx);
	void setArrayElement(uint32_t idx, const CScriptVarPtr &value);
private:
//	void native_getLength(const CFunctionsScopePtr &c, void *data);
//	void native_setLength(const CFunctionsScopePtr &c, void *data);
	bool toStringRecursion;
//	uint32_t length;
	friend define_newScriptVar_Fnc(Array, CTinyJS *Context, Array_t);
};
inline define_newScriptVar_Fnc(Array, CTinyJS *Context, Array_t) { return new CScriptVarArray(Context); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarRegExp
//////////////////////////////////////////////////////////////////////////
#ifndef NO_REGEXP

define_ScriptVarPtr_Type(RegExp);
class CScriptVarRegExp : public CScriptVarObject {
protected:
	CScriptVarRegExp(CTinyJS *Context, const std::string &Source, const std::string &Flags);
	CScriptVarRegExp(const CScriptVarRegExp& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarRegExp() OVERRIDE;
	virtual bool isRegExp() OVERRIDE; // { return true; }
	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;

	CScriptVarPtr exec(const std::string &Input, bool Test=false);

	bool Global() { return flags.find('g')!=std::string::npos; }
	bool IgnoreCase() { return flags.find('i')!=std::string::npos; }
	bool Multiline() { return true; /* currently always true -- flags.find('m')!=std::string::npos;*/ }
	bool Sticky() { return flags.find('y')!=std::string::npos; }
	const std::string &Regexp() { return regexp; }
	unsigned int LastIndex();

	static const char *ErrorStr(int Error);
protected:
	std::string regexp;
	std::string flags;
private:
	void native_Global(const CFunctionsScopePtr &c, void *data);
	void native_IgnoreCase(const CFunctionsScopePtr &c, void *data);
	void native_Multiline(const CFunctionsScopePtr &c, void *data);
	void native_Sticky(const CFunctionsScopePtr &c, void *data);
	void native_Source(const CFunctionsScopePtr &c, void *data);

	friend define_newScriptVar_Fnc(RegExp, CTinyJS *Context, const std::string &, const std::string &);

};
inline define_newScriptVar_Fnc(RegExp, CTinyJS *Context, const std::string &Obj, const std::string &Flags) { return new CScriptVarRegExp(Context, Obj, Flags); }

#endif /* NO_REGEXP */


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunction
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(Function);
class CScriptVarFunction : public CScriptVarObject {
protected:
	CScriptVarFunction(CTinyJS *Context, CScriptTokenDataFnc *Data);
	CScriptVarFunction(const CScriptVarFunction& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarFunction() OVERRIDE;
	virtual bool isObject() OVERRIDE; // { return true; }
	virtual bool isFunction() OVERRIDE; // { return true; }
	virtual bool isPrimitive() OVERRIDE; // { return false; }

	virtual std::string getVarType() OVERRIDE; // { return "function"; }
	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) OVERRIDE;
	virtual CScriptVarPtr toString_CallBack(CScriptResult &execute, int radix=0) OVERRIDE;

	virtual void setTemporaryMark_recursive(uint32_t ID) OVERRIDE;

	virtual CScriptTokenDataFnc *getFunctionData();
	void setFunctionData(CScriptTokenDataFnc *Data);
	void setConstructor(const CScriptVarFunctionPtr &Constructor) { constructor = Constructor; };
	const CScriptVarFunctionPtr &getConstructor() { return constructor; }
private:
	CScriptTokenDataFnc *data;
	CScriptVarFunctionPtr constructor;

	friend define_newScriptVar_Fnc(Function, CTinyJS *Context, CScriptTokenDataFnc *);
};
inline define_newScriptVar_Fnc(Function, CTinyJS *Context, CScriptTokenDataFnc *Obj) { return new CScriptVarFunction(Context, Obj); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionBounded
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(FunctionBounded);
class CScriptVarFunctionBounded : public CScriptVarFunction {
protected:
	CScriptVarFunctionBounded(CScriptVarFunctionPtr BoundedFunction, CScriptVarPtr BoundedThis, const std::vector<CScriptVarPtr> &BoundedArguments);
	CScriptVarFunctionBounded(const CScriptVarFunctionBounded& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarFunctionBounded() OVERRIDE;
	virtual bool isBounded() OVERRIDE;	///< is CScriptVarFunctionBounded
	virtual void setTemporaryMark_recursive(uint32_t ID) OVERRIDE;
	CScriptVarPtr callFunction(CScriptResult &execute, std::vector<CScriptVarPtr> &Arguments, const CScriptVarPtr &This, CScriptVarPtr *newThis=0);
protected:
private:
	CScriptVarFunctionPtr boundedFunction;
	CScriptVarPtr boundedThis;
	std::vector<CScriptVarPtr> boundedArguments;

	friend define_newScriptVar_NamedFnc(FunctionBounded, CScriptVarFunctionPtr BoundedFunction, CScriptVarPtr BoundedThis, const std::vector<CScriptVarPtr> &BoundedArguments);
};
inline define_newScriptVar_NamedFnc(FunctionBounded, CScriptVarFunctionPtr BoundedFunction, CScriptVarPtr BoundedThis, const std::vector<CScriptVarPtr> &BoundedArguments) { return new CScriptVarFunctionBounded(BoundedFunction, BoundedThis, BoundedArguments); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionNative
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(FunctionNative);
class CScriptVarFunctionNative : public CScriptVarFunction {
protected:
	CScriptVarFunctionNative(CTinyJS *Context, void *Userdata, const char *Name, const char *Args);
	CScriptVarFunctionNative(const CScriptVarFunctionNative& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarFunctionNative() OVERRIDE;
	virtual bool isNative() OVERRIDE; // { return true; }

	virtual void callFunction(const CFunctionsScopePtr &c)=0;// { jsCallback(c, jsCallbackUserData); }
protected:
	void *jsUserData; ///< user data passed as second argument to native functions
};


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionNativeCallback
//////////////////////////////////////////////////////////////////////////

define_ScriptVarPtr_Type(FunctionNativeCallback);
class CScriptVarFunctionNativeCallback : public CScriptVarFunctionNative {
protected:
	CScriptVarFunctionNativeCallback(CTinyJS *Context, JSCallback Callback, void *Userdata, const char *Name, const char *Args) : CScriptVarFunctionNative(Context, Userdata, Name, Args), jsCallback(Callback) {}
	CScriptVarFunctionNativeCallback(const CScriptVarFunctionNativeCallback& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarFunctionNativeCallback() OVERRIDE;
	virtual void callFunction(const CFunctionsScopePtr &c) OVERRIDE;
private:
	JSCallback jsCallback; ///< Callback for native functions
	friend define_newScriptVar_Fnc(FunctionNativeCallback, CTinyJS *Context, JSCallback Callback, void*, const char*, const char*);
};
inline define_newScriptVar_Fnc(FunctionNativeCallback, CTinyJS *Context, JSCallback Callback, void *Userdata, const char *Name=0, const char *Args=0) { return new CScriptVarFunctionNativeCallback(Context, Callback, Userdata, Name, Args); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionNativeClass
//////////////////////////////////////////////////////////////////////////

template<class native>
class CScriptVarFunctionNativeClass : public CScriptVarFunctionNative {
protected:
	CScriptVarFunctionNativeClass(CTinyJS *Context, native *ClassPtr, void (native::*ClassFnc)(const CFunctionsScopePtr &, void *), void *Userdata, const char *Name, const char *Args) : CScriptVarFunctionNative(Context, Userdata, Name, Args), classPtr(ClassPtr), classFnc(ClassFnc) {}
	CScriptVarFunctionNativeClass(const CScriptVarFunctionNativeClass& Copy) MEMBER_DELETE;
public:

	virtual void callFunction(const CFunctionsScopePtr &c) OVERRIDE { (classPtr->*classFnc)(c, jsUserData); }
private:
	native *classPtr;
	void (native::*classFnc)(const CFunctionsScopePtr &c, void *userdata);
	template<typename native2>
	friend define_newScriptVar_Fnc(FunctionNativeCallback, CTinyJS*, native2 *, void (native2::*)(const CFunctionsScopePtr &, void *), void *, const char *, const char *);
};
template<typename native>
define_newScriptVar_Fnc(FunctionNativeCallback, CTinyJS *Context, native *ClassPtr, void (native::*ClassFnc)(const CFunctionsScopePtr &, void *), void *Userdata, const char *Name=0, const char *Args=0) { return new CScriptVarFunctionNativeClass<native>(Context, ClassPtr, ClassFnc, Userdata, Name, Args); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarAccessor
//////////////////////////////////////////////////////////////////////////

define_dummy_t(Accessor);
define_ScriptVarPtr_Type(Accessor);

class CScriptVarAccessor : public CScriptVarObject {
protected:
	CScriptVarAccessor(CTinyJS *Context);
	CScriptVarAccessor(CTinyJS *Context, JSCallback getter, void *getterdata, JSCallback setter, void *setterdata);
	template<class C>	CScriptVarAccessor(CTinyJS *Context, C *class_ptr, void(C::*getterFnc)(const CFunctionsScopePtr &, void *), void *getterData, void(C::*setterFnc)(const CFunctionsScopePtr &, void *), void *setterData) : CScriptVarObject(Context) {
		if(getterFnc)
			addChild(TINYJS_ACCESSOR_GET_VAR, ::newScriptVar(Context, class_ptr, getterFnc, getterData), 0);
		if(setterFnc)
			addChild(TINYJS_ACCESSOR_SET_VAR, ::newScriptVar(Context, class_ptr, setterFnc, setterData), 0);
	}
	CScriptVarAccessor(CTinyJS *Context, const CScriptVarFunctionPtr &getter, const CScriptVarFunctionPtr &setter);

	CScriptVarAccessor(const CScriptVarAccessor& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarAccessor() OVERRIDE;
	virtual bool isAccessor() OVERRIDE; // { return true; }
	virtual bool isPrimitive() OVERRIDE; // { return false; }

	virtual std::string getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) OVERRIDE;
	virtual std::string getVarType() OVERRIDE; // { return "object"; }

	CScriptVarPtr getValue();

	friend define_newScriptVar_Fnc(Accessor, CTinyJS *Context, Accessor_t);
	friend define_newScriptVar_NamedFnc(Accessor, CTinyJS *Context, JSCallback getter, void *getterdata, JSCallback setter, void *setterdata);
	template<class C> friend define_newScriptVar_NamedFnc(Accessor, CTinyJS *Context, C *class_ptr, void(C::*getterFnc)(const CFunctionsScopePtr &, void *), void *getterData, void(C::*setterFnc)(const CFunctionsScopePtr &, void *), void *setterData);
	friend define_newScriptVar_NamedFnc(Accessor, CTinyJS *Context, const CScriptVarFunctionPtr &, const CScriptVarFunctionPtr &);
};
inline define_newScriptVar_Fnc(Accessor, CTinyJS *Context, Accessor_t) { return new CScriptVarAccessor(Context); }
inline define_newScriptVar_NamedFnc(Accessor, CTinyJS *Context, JSCallback getter, void *getterdata, JSCallback setter, void *setterdata) { return new CScriptVarAccessor(Context, getter, getterdata, setter, setterdata); }
template<class C> define_newScriptVar_NamedFnc(Accessor, CTinyJS *Context, C *class_ptr, void(C::*getterFnc)(const CFunctionsScopePtr &, void *), void *getterData, void(C::*setterFnc)(const CFunctionsScopePtr &, void *), void *setterData)  { return new CScriptVarAccessor(Context, class_ptr, getterFnc, getterData, setterFnc, setterData); }
inline define_newScriptVar_NamedFnc(Accessor, CTinyJS *Context, const CScriptVarFunctionPtr &getter, const CScriptVarFunctionPtr &setter) { return new CScriptVarAccessor(Context, getter, setter); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarDestructuring
//////////////////////////////////////////////////////////////////////////

class CScriptVarDestructuring : public CScriptVarObject {
protected: // only derived classes or friends can be created
	CScriptVarDestructuring(CTinyJS *Context) // constructor for rootScope
		: CScriptVarObject(Context) {}
public:
	virtual ~CScriptVarDestructuring() OVERRIDE;
};

//////////////////////////////////////////////////////////////////////////
/// CScriptVarScope
//////////////////////////////////////////////////////////////////////////

define_dummy_t(Scope);
define_ScriptVarPtr_Type(Scope);
class CScriptVarScope : public CScriptVarObject {
protected: // only derived classes or friends can be created
	CScriptVarScope(CTinyJS *Context) // constructor for rootScope
		: CScriptVarObject(Context) {}
	virtual bool isObject() OVERRIDE; // { return false; }
public:
	virtual ~CScriptVarScope() OVERRIDE;
	virtual CScriptVarPtr scopeVar(); ///< to create var like: var a = ...
	virtual CScriptVarPtr scopeLet(); ///< to create var like: let a = ...
	virtual CScriptVarLinkWorkPtr findInScopes(const std::string &childName);
	virtual CScriptVarScopePtr getParent();
	friend define_newScriptVar_Fnc(Scope, CTinyJS *Context, Scope_t);
};
inline define_newScriptVar_Fnc(Scope, CTinyJS *Context, Scope_t) { return new CScriptVarScope(Context); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarScopeFnc
//////////////////////////////////////////////////////////////////////////

define_dummy_t(ScopeFnc);
define_ScriptVarPtr_Type(ScopeFnc);
class CScriptVarScopeFnc : public CScriptVarScope {
protected: // only derived classes or friends can be created
	CScriptVarScopeFnc(CTinyJS *Context, const CScriptVarScopePtr &Closure) // constructor for FncScope
		: CScriptVarScope(Context), closure(Closure ? addChild(TINYJS_FUNCTION_CLOSURE_VAR, Closure, 0) : CScriptVarLinkPtr()) {}
public:
	virtual ~CScriptVarScopeFnc() OVERRIDE;
	virtual CScriptVarLinkWorkPtr findInScopes(const std::string &childName) OVERRIDE;

	void setReturnVar(const CScriptVarPtr &var); ///< Set the result value. Use this when setting complex return data as it avoids a deepCopy()

	#define DEPRECATED_getParameter DEPRECATED("getParameter is deprecated use getArgument instead")
	DEPRECATED_getParameter CScriptVarPtr getParameter(const std::string &name);
	DEPRECATED_getParameter CScriptVarPtr getParameter(int Idx);
	CScriptVarPtr getArgument(const std::string &name); ///< If this is a function, get the parameter with the given name (for use by native functions)
	CScriptVarPtr getArgument(int Idx); ///< If this is a function, get the parameter with the given index (for use by native functions)
	DEPRECATED("getParameterLength is deprecated use getArgumentsLength instead") int getParameterLength(); ///< If this is a function, get the count of parameters
	uint32_t getArgumentsLength(); ///< If this is a function, get the count of parameters

	void throwError(ERROR_TYPES ErrorType, const std::string &message);
	void throwError(ERROR_TYPES ErrorType, const char *message);

	void assign(CScriptVarLinkWorkPtr &lhs, CScriptVarPtr rhs, bool ignoreReadOnly=false, bool ignoreNotOwned=false, bool ignoreNotExtensible=false);
	CScriptVarLinkWorkPtr getProperty(const CScriptVarPtr &Objc, const std::string &name) { return Objc->findChildWithPrototypeChain(name); }
	CScriptVarLinkWorkPtr getProperty(const CScriptVarPtr &Objc, uint32_t idx)  { return Objc->findChildWithPrototypeChain(int2string(idx)); }
	CScriptVarPtr getPropertyValue(const CScriptVarPtr &Objc, const std::string &name) { return getProperty(Objc, name).getter(); } // short for getProperty().getter()
	CScriptVarPtr getPropertyValue(const CScriptVarPtr &Objc, uint32_t idx) { return getProperty(Objc, idx).getter(); } // short for getProperty().getter()
	uint32_t getLength(const CScriptVarPtr &Objc) { return Objc->getLength(); }
	void setProperty(const CScriptVarPtr &Objc, const std::string &name, const CScriptVarPtr &_value, bool ignoreReadOnly=false, bool ignoreNotExtensible=false) {
		CScriptVarLinkWorkPtr property = getProperty(Objc, name);
		setProperty(property, _value, ignoreReadOnly, ignoreNotExtensible);
	}
	void setProperty(const CScriptVarPtr &Objc, uint32_t idx, const CScriptVarPtr &_value, bool ignoreReadOnly=false, bool ignoreNotExtensible=false) {
		setProperty(Objc, int2string(idx), _value, ignoreReadOnly, ignoreNotExtensible);
	}
	void setProperty(CScriptVarLinkWorkPtr &lhs, const CScriptVarPtr &rhs, bool ignoreReadOnly=false, bool ignoreNotExtensible=false, bool ignoreNotOwned=false);

protected:
	CScriptVarLinkPtr closure;
	friend define_newScriptVar_Fnc(ScopeFnc, CTinyJS *Context, ScopeFnc_t, const CScriptVarScopePtr &Closure);
};
inline define_newScriptVar_Fnc(ScopeFnc, CTinyJS *Context, ScopeFnc_t, const CScriptVarScopePtr &Closure) { return new CScriptVarScopeFnc(Context, Closure); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarScopeLet
//////////////////////////////////////////////////////////////////////////

define_dummy_t(ScopeLet);
define_ScriptVarPtr_Type(ScopeLet);
class CScriptVarScopeLet : public CScriptVarScope {
protected: // only derived classes or friends can be created
	CScriptVarScopeLet(const CScriptVarScopePtr &Parent); // constructor for LetScope
//		: CScriptVarScope(Parent->getContext()), parent( context->getRoot() != Parent ? addChild(TINYJS_SCOPE_PARENT_VAR, Parent, 0) : 0) {}
public:
	virtual ~CScriptVarScopeLet() OVERRIDE;
	virtual CScriptVarLinkWorkPtr findInScopes(const std::string &childName) OVERRIDE;
	virtual CScriptVarPtr scopeVar() OVERRIDE; ///< to create var like: var a = ...
	virtual CScriptVarScopePtr getParent() OVERRIDE;
	void setletExpressionInitMode(bool Mode) { letExpressionInitMode = Mode; }
protected:
	CScriptVarLinkPtr parent;
	bool letExpressionInitMode;
	friend define_newScriptVar_Fnc(ScopeLet, CTinyJS *Context, ScopeLet_t, const CScriptVarScopePtr &Parent);
};
inline define_newScriptVar_Fnc(ScopeLet, CTinyJS *, ScopeLet_t, const CScriptVarScopePtr &Parent) { return new CScriptVarScopeLet(Parent); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarScopeWith
//////////////////////////////////////////////////////////////////////////

define_dummy_t(ScopeWith);
define_ScriptVarPtr_Type(ScopeWith);
class CScriptVarScopeWith : public CScriptVarScopeLet {
protected:
	CScriptVarScopeWith(const CScriptVarScopePtr &Parent, const CScriptVarPtr &With)
		: CScriptVarScopeLet(Parent), with(addChild(TINYJS_SCOPE_WITH_VAR, With, 0)) {}

public:
	virtual ~CScriptVarScopeWith() OVERRIDE;
	virtual CScriptVarPtr scopeLet() OVERRIDE; ///< to create var like: let a = ...
	virtual CScriptVarLinkWorkPtr findInScopes(const std::string &childName) OVERRIDE;
private:
	CScriptVarLinkPtr with;
	friend define_newScriptVar_Fnc(ScopeWith, CTinyJS *Context, ScopeWith_t, const CScriptVarScopePtr &Parent, const CScriptVarPtr &With);
};
inline define_newScriptVar_Fnc(ScopeWith, CTinyJS *, ScopeWith_t, const CScriptVarScopePtr &Parent, const CScriptVarPtr &With) { return new CScriptVarScopeWith(Parent, With); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarDefaultIterator
//////////////////////////////////////////////////////////////////////////

define_dummy_t(DefaultIterator);
define_ScriptVarPtr_Type(DefaultIterator);

class CScriptVarDefaultIterator : public CScriptVarObject {
protected:
	CScriptVarDefaultIterator(CTinyJS *Context, const CScriptVarPtr &Object, IteratorMode Mode);
	CScriptVarDefaultIterator(const CScriptVarDefaultIterator& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarDefaultIterator() OVERRIDE;
	virtual bool isIterator() OVERRIDE;

	void native_next(const CFunctionsScopePtr &c, void *data);
private:
	IteratorMode mode;
	CScriptVarPtr object;
	STRING_SET_t keys;
	STRING_SET_it pos;
	friend define_newScriptVar_NamedFnc(DefaultIterator, CTinyJS *, const CScriptVarPtr &, IteratorMode);

};
inline define_newScriptVar_NamedFnc(DefaultIterator, CTinyJS *Context, const CScriptVarPtr &_Object, IteratorMode Mode) { return new CScriptVarDefaultIterator(Context, _Object, Mode); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarGenerator
//////////////////////////////////////////////////////////////////////////

#ifndef NO_GENERATORS

define_dummy_t(Generator);
define_ScriptVarPtr_Type(Generator);

class CScriptVarGenerator : public CScriptVarObject {
protected:
	CScriptVarGenerator(CTinyJS *Context, const CScriptVarPtr &FunctionRoot, const CScriptVarFunctionPtr &Function);
	CScriptVarGenerator(const CScriptVarGenerator& Copy) MEMBER_DELETE;
public:
	virtual ~CScriptVarGenerator() OVERRIDE;
	virtual bool isIterator() OVERRIDE;
	virtual bool isGenerator() OVERRIDE;
	virtual std::string getVarType() OVERRIDE; // { return "generator"; }
	virtual std::string getVarTypeTagName() OVERRIDE; // { return "Generator"; }

	CScriptVarPtr getFunctionRoot() { return functionRoot; }
	CScriptVarFunctionPtr getFunction() { return function; }

	virtual void setTemporaryMark_recursive(uint32_t ID) OVERRIDE;

	void native_send(const CFunctionsScopePtr &c, void *data);
	void native_throw(const CFunctionsScopePtr &c, void *data);
	int Coroutine();
	void setException(const CScriptVarPtr &YieldVar) {
		yieldVar = YieldVar;
		yieldVarIsException = true;
	}
	bool isClosed() const { return closed; }
	CScriptVarPtr yield(CScriptResult &execute, CScriptVar *YieldIn);
private:
	CScriptVarPtr functionRoot;
	CScriptVarFunctionPtr function;
	bool closed;
	CScriptVarPtr yieldVar;
	bool yieldVarIsException;
	class CCooroutine : public CScriptCoroutine {
		CCooroutine(CScriptVarGenerator* Parent) : parent(Parent) {}
		int Coroutine() { return parent->Coroutine(); }
		void yield() { CScriptCoroutine::yield(); }
		CScriptVarGenerator* parent;
		friend class CScriptVarGenerator;
	} coroutine;
	friend class CCooroutine;

public:
	void *callersStackBase;
	size_t callersScopeSize;
	CScriptTokenizer *callersTokenizer;
	bool callersHaveTry;
	std::vector<CScriptVarScopePtr> generatorScopes;

	friend define_newScriptVar_NamedFnc(CScriptVarGenerator, CTinyJS *, const CScriptVarPtr &, const CScriptVarFunctionPtr &);

};
inline define_newScriptVar_NamedFnc(CScriptVarGenerator, CTinyJS *Context, const CScriptVarPtr &FunctionRoot, const CScriptVarFunctionPtr &Function) { return new CScriptVarGenerator(Context, FunctionRoot, Function); }

#endif

//////////////////////////////////////////////////////////////////////////
template<typename T>
inline CScriptVarPtr CScriptVar::newScriptVar(T t) { return ::newScriptVar(context, t); }
template<typename T1, typename T2>
inline CScriptVarPtr CScriptVar::newScriptVar(T1 t1, T2 t2) { return ::newScriptVar(context, t1, t2); }
//inline CScriptVarPtr newScriptVar(const CNumber &t) { return ::newScriptVar(context, t); }
//////////////////////////////////////////////////////////////////////////


class CScriptResult {
public:
	enum TYPE {
		Normal,
		Break,
		Continue,
		Return,
		Throw,
		noncatchableThrow,
		noExecute
	};
	CScriptResult(TYPE Type=Normal) : type(Type), throw_at_line(-1), throw_at_column(-1), strictMode(false) {}
//	CScriptResult(TYPE Type) : type(Type), throw_at_line(-1), throw_at_column(-1) {}
//		~RESULT() { if(type==Throw) throw value; }
	bool isNormal() const { return type == Normal; }
	bool isBreak() const { return type == Break; }
	bool isContinue() const { return type == Continue; }
	bool isBreakContinue() const { return type == Break || type == Continue; }
	bool isReturn() const { return type == Return; }
	bool isReturnNormal() const { return type == Return || type == Normal; }
	bool isThrow() const { return type == Throw; }

	bool useStrict() const { return strictMode; }

	operator bool() const { return type==Normal; }
	void set(TYPE Type, bool Clear=true) { type=Type; if(Clear) value.clear(), target.clear(); }
	void set(TYPE Type, const CScriptVarPtr &Value) { type=Type; value=Value; }
	void set(TYPE Type, const std::string &Target) { type=Type; target=Target; }
	void setThrow(const CScriptVarPtr &Value, const std::string &File, int Line=-1, int Column=-1) { type=Throw; value=Value; throw_at_file=File, throw_at_line=Line; throw_at_column=Column; }

	void cThrow() const { if (type == Throw) throw value; }

	// the operator is is less intuitive use updateIsNotNormal instead
	CScriptResult DEPRECATED("this operator is deprecated use updateIsNotNormal instead")& operator()(const CScriptResult& rhs) { if (rhs.type != Normal) *this = rhs; return *this; }
	CScriptResult& updateIsNotNormal(const CScriptResult& rhs) { if (rhs.type != Normal) *this = rhs; return *this; }

	enum TYPE type;
	CScriptVarPtr value;
	std::string target;
	std::string throw_at_file;
	int throw_at_line;
	int throw_at_column;
	bool strictMode;
};



//////////////////////////////////////////////////////////////////////////
/// CTinyJS
//////////////////////////////////////////////////////////////////////////

typedef int (*native_require_read_fnc)(const std::string &Fname, std::string &Data);

class CTinyJS {
public:
	CTinyJS();
	~CTinyJS();

	void execute(CScriptTokenizer &Tokenizer);
	void execute(const char *Code, const std::string &File="", int Line=0, int Column=0);
	void execute(const std::string &Code, const std::string &File="", int Line=0, int Column=0);
	/** Evaluate the given code and return a link to a javascript object,
	 * useful for (dangerous) JSON parsing. If nothing to return, will return
	 * 'undefined' variable type. CScriptVarLink is returned as this will
	 * automatically unref the result as it goes out of scope. If you want to
	 * keep it, you must use ref() and unref() */
	CScriptVarLinkPtr evaluateComplex(CScriptTokenizer &Tokenizer);
	/** Evaluate the given code and return a link to a javascript object,
	 * useful for (dangerous) JSON parsing. If nothing to return, will return
	 * 'undefined' variable type. CScriptVarLink is returned as this will
	 * automatically unref the result as it goes out of scope. If you want to
	 * keep it, you must use ref() and unref() */
	CScriptVarLinkPtr evaluateComplex(const char *code, const std::string &File="", int Line=0, int Column=0);
	/** Evaluate the given code and return a link to a javascript object,
	 * useful for (dangerous) JSON parsing. If nothing to return, will return
	 * 'undefined' variable type. CScriptVarLink is returned as this will
	 * automatically unref the result as it goes out of scope. If you want to
	 * keep it, you must use ref() and unref() */
	CScriptVarLinkPtr evaluateComplex(const std::string &code, const std::string &File="", int Line=0, int Column=0);
	/** Evaluate the given code and return a string. If nothing to return, will return
	 * 'undefined' */
	std::string evaluate(CScriptTokenizer &Tokenizer);
	/** Evaluate the given code and return a string. If nothing to return, will return
	 * 'undefined' */
	std::string evaluate(const char *code, const std::string &File="", int Line=0, int Column=0);
	/** Evaluate the given code and return a string. If nothing to return, will return
	 * 'undefined' */
	std::string evaluate(const std::string &code, const std::string &File="", int Line=0, int Column=0);

	native_require_read_fnc setRequireReadFnc(native_require_read_fnc fnc) {
		native_require_read_fnc old = native_require_read;
		native_require_read = fnc;
		return old;
	}

	/// add a native function to be called from TinyJS
	/** example:
		\code
			void scRandInt(const CFunctionsScopePtr &c, void *userdata) { ... }
			tinyJS->addNative("function randInt(min, max)", scRandInt, 0);
		\endcode

		or

		\code
			void scSubstring(const CFunctionsScopePtr &c, void *userdata) { ... }
			tinyJS->addNative("function String.substring(lo, hi)", scSubstring, 0);
		\endcode
		or

		\code
			class Class
			{
			public:
				void scSubstring(const CFunctionsScopePtr &c, void *userdata) { ... }
			};
			Class Instanz;
			tinyJS->addNative("function String.substring(lo, hi)", &Instanz, &Class::*scSubstring, 0);
		\endcode
	*/
private:
	CScriptVarPtr addNative_ParseFuncDesc(const std::string &funcDesc, std::string &name, std::string &args);
public:
	CScriptVarFunctionNativePtr addNative(const std::string &funcDesc, JSCallback ptr, void *userdata=0, int LinkFlags=SCRIPTVARLINK_BUILDINDEFAULT);
	template<class C>
	CScriptVarFunctionNativePtr addNative(const std::string &funcDesc, C *class_ptr, void(C::*class_fnc)(const CFunctionsScopePtr &, void *), void *userdata=0, int LinkFlags=SCRIPTVARLINK_BUILDINDEFAULT)
	{
		std::string name, args;
		CScriptVarPtr ret, base = addNative_ParseFuncDesc(funcDesc, name, args);
		base->addChild(name, ret = ::newScriptVar<C>(this, class_ptr, class_fnc, userdata, name.c_str(), args.c_str()), LinkFlags);;
		return ret;
	}

	/// Send all variables to stdout
	void trace();

	const CScriptVarScopePtr &getRoot() { return root; };   /// gets the root of symbol table
	//	CScriptVar *root;   /// root of symbol table

	/// newVars & constVars
	//CScriptVarPtr newScriptVar(const CNumber &t) { return ::newScriptVar(this, t); }
	template<typename T>	CScriptVarPtr newScriptVar(T t) { return ::newScriptVar(this, t); }
	template<typename T1, typename T2>	CScriptVarPtr newScriptVar(T1 t1, T2 t2) { return ::newScriptVar(this, t1, t2); }
	template<typename T1, typename T2, typename T3>	CScriptVarPtr newScriptVar(T1 t1, T2 t2, T3 t3) { return ::newScriptVar(this, t1, t2, t3); }
	const CScriptVarPtr &constScriptVar(Undefined_t)		{ return constUndefined; }
	const CScriptVarPtr &constScriptVar(Null_t)				{ return constNull; }
	const CScriptVarPtr &constScriptVar(NaN_t)				{ return constNaN; }
	const CScriptVarPtr &constScriptVar(Infinity _t)		{ return _t.Sig()<0 ? constInfinityNegative : constInfinityPositive; }
	const CScriptVarPtr &constScriptVar(bool Val)			{ return Val?constTrue:constFalse; }
	const CScriptVarPtr &constScriptVar(NegativeZero_t)		{ return constNegativZero; }
	const CScriptVarPtr &constScriptVar(StopIteration_t)	{ return constStopIteration; }

private:
	CScriptTokenizer *t;       /// current tokenizer
	bool haveTry;
	std::vector<CScriptVarScopePtr>scopes;
	CScriptVarScopePtr root;
	const CScriptVarScopePtr &scope() { return scopes.back(); }

	class CScopeControl { // helper-class to manage scopes
	private:
		CScopeControl(const CScopeControl& Copy) MEMBER_DELETE; // no copy
		CScopeControl& operator =(const CScopeControl& Copy) MEMBER_DELETE;
	public:
		CScopeControl(CTinyJS *Context) : context(Context), count(0) {}
		~CScopeControl() { clear(); }
		void clear() { while(count--) {CScriptVarScopePtr parent = context->scopes.back()->getParent(); if(parent) context->scopes.back() = parent; else context->scopes.pop_back() ;} count=0; }
		void addFncScope(const CScriptVarScopePtr &_Scope) { context->scopes.push_back(_Scope); count++; }
		CScriptVarScopeLetPtr addLetScope() {	count++; return context->scopes.back() = ::newScriptVar(context, ScopeLet, context->scopes.back()); }
		void addWithScope(const CScriptVarPtr &With) { context->scopes.back() = ::newScriptVar(context, ScopeWith, context->scopes.back(), With); count++; }
	private:
		CTinyJS *context;
		int		count;
	};
	friend class CScopeControl;
public:
	CScriptVarPtr objectPrototype; /// Built in object class
	CScriptVarPtr objectPrototype_valueOf; /// Built in object class
	CScriptVarPtr objectPrototype_toString; /// Built in object class
	CScriptVarPtr arrayPrototype; /// Built in array class
	CScriptVarPtr stringPrototype; /// Built in string class
	CScriptVarPtr regexpPrototype; /// Built in string class
	CScriptVarPtr numberPrototype; /// Built in number class
	CScriptVarPtr booleanPrototype; /// Built in boolean class
	CScriptVarPtr iteratorPrototype; /// Built in iterator class
#ifndef NO_GENERATORS
	CScriptVarPtr generatorPrototype; /// Built in generator class
#endif /*NO_GENERATORS*/
	CScriptVarPtr functionPrototype; /// Built in function class
	const CScriptVarPtr &getErrorPrototype(ERROR_TYPES Type) { return errorPrototypes[Type]; }
private:
	CScriptVarPtr errorPrototypes[ERROR_COUNT]; /// Built in error class
	CScriptVarPtr constUndefined;
	CScriptVarPtr constNull;
	CScriptVarPtr constNaN;
	CScriptVarPtr constInfinityPositive;
	CScriptVarPtr constInfinityNegative;
	CScriptVarPtr constNegativZero;
	CScriptVarPtr constTrue;
	CScriptVarPtr constFalse;
	CScriptVarPtr constStopIteration;

	std::vector<CScriptVarPtr *> pseudo_refered;

	void CheckRightHandVar(CScriptResult &execute, CScriptVarLinkWorkPtr &link)
	{
		if(execute && link && !link->isOwned() && !link.hasReferencedOwner() && !link->getName().empty())
			throwError(execute, ReferenceError, link->getName() + " is not defined", t->getPrevPos());
	}

	void CheckRightHandVar(CScriptResult &execute, CScriptVarLinkWorkPtr &link, CScriptTokenizer::ScriptTokenPosition &Pos)
	{
		if(execute && link && !link->isOwned() && !link.hasReferencedOwner() && !link->getName().empty())
			throwError(execute, ReferenceError, link->getName() + " is not defined", Pos);
	}

public:
	// function call
	CScriptVarPtr callFunction(const CScriptVarFunctionPtr &Function, std::vector<CScriptVarPtr> &Arguments, const CScriptVarPtr &This=0, CScriptVarPtr *newThis=0);
	CScriptVarPtr callFunction(CScriptResult &execute, const CScriptVarFunctionPtr &Function, std::vector<CScriptVarPtr> &Arguments, const CScriptVarPtr &This, CScriptVarPtr *newThis=0);
	//////////////////////////////////////////////////////////////////////////
#ifndef NO_GENERATORS
	std::vector<CScriptVarGenerator *> generatorStack;
	void generator_start(CScriptVarGenerator *Generator);
	CScriptVarPtr generator_yield(CScriptResult &execute, CScriptVar *YieldIn);
#endif
	//////////////////////////////////////////////////////////////////////////

	// parsing - in order of precedence
	CScriptVarPtr mathsOp(CScriptResult &execute, const CScriptVarPtr &a, const CScriptVarPtr &b, int op);
private:
	void assign_destructuring_var(CScriptResult &execute, const CScriptTokenDataDestructuringVar &Objc, const CScriptVarPtr &Val, const CScriptVarPtr &Scope);
	void execute_var_init(CScriptResult &execute, bool hideLetScope);
	void execute_destructuring(CScriptResult &execute, CScriptTokenDataObjectLiteral &Objc, const CScriptVarPtr &Val, const std::string &Path);
	CScriptVarLinkWorkPtr execute_literals(CScriptResult &execute);
	CScriptVarLinkWorkPtr execute_member(CScriptVarLinkWorkPtr &parent, CScriptResult &execute);
	CScriptVarLinkWorkPtr execute_function_call(CScriptResult &execute);
	bool execute_unary_rhs(CScriptResult &execute, CScriptVarLinkWorkPtr& a);
	CScriptVarLinkWorkPtr execute_unary(CScriptResult &execute);
	CScriptVarLinkWorkPtr execute_exponentiation(CScriptResult& execute);
	CScriptVarLinkWorkPtr execute_term(CScriptResult &execute);
	CScriptVarLinkWorkPtr execute_expression(CScriptResult &execute);
	CScriptVarLinkWorkPtr execute_binary_shift(CScriptResult &execute);
	CScriptVarLinkWorkPtr execute_relation(CScriptResult &execute, int set=LEX_EQUAL, int set_n='<');
	CScriptVarLinkWorkPtr execute_binary_logic(CScriptResult &execute, int op='|', int op_n1='^', int op_n2='&');
	CScriptVarLinkWorkPtr execute_logic(CScriptResult &execute, int op=LEX_OROR, int op_n=LEX_ANDAND);
	CScriptVarLinkWorkPtr execute_condition(CScriptResult &execute);
	CScriptVarLinkPtr execute_assignment(CScriptVarLinkWorkPtr Lhs, CScriptResult &execute);
	CScriptVarLinkPtr execute_assignment(CScriptResult &execute);
	CScriptVarLinkPtr execute_base(CScriptResult &execute);
	void execute_block(CScriptResult &execute);
	void execute_statement(CScriptResult &execute);
	// parsing utility functions
	CScriptVarLinkWorkPtr parseFunctionDefinition(const CScriptToken &FncToken);
	CScriptVarLinkWorkPtr parseFunctionsBodyFromString(const std::string &ArgumentList, const std::string &FncBody);
public:
	CScriptVarLinkPtr findInScopes(const std::string &childName); ///< Finds a child, looking recursively up the scopes
private:
	//////////////////////////////////////////////////////////////////////////
	/// addNative-helper
	CScriptVarFunctionNativePtr addNative(const std::string &funcDesc, CScriptVarFunctionNativePtr Var, int LinkFlags);

	//////////////////////////////////////////////////////////////////////////
	/// throws an Error & Exception
public:
	void throwError(CScriptResult &execute, ERROR_TYPES ErrorType, const std::string &message);
	void throwException(ERROR_TYPES ErrorType, const std::string &message);
	void throwError(CScriptResult &execute, ERROR_TYPES ErrorType, const std::string &message, CScriptTokenizer::ScriptTokenPosition &Pos);
	void throwException(ERROR_TYPES ErrorType, const std::string &message, CScriptTokenizer::ScriptTokenPosition &Pos);
private:
	//////////////////////////////////////////////////////////////////////////
	/// native Object-Constructors & prototype-functions

	void native_Object(const CFunctionsScopePtr &c, void *data);
	void native_Object_getPrototypeOf(const CFunctionsScopePtr &c, void *data);
	void native_Object_setPrototypeOf(const CFunctionsScopePtr &c, void *data);
	void native_Object_prototype_getter__proto__(const CFunctionsScopePtr &c, void *data);
	void native_Object_prototype_setter__proto__(const CFunctionsScopePtr &c, void *data);
	/* Userdate for set-/isObjectState
	 * 0 - preventExtensions / isExtensible
	 * 1 - seal / isSealed
	 * 2 - freeze / isFrozen
	 */
	void native_Object_setObjectSecure(const CFunctionsScopePtr &c, void *data);
	void native_Object_isSecureObject(const CFunctionsScopePtr &c, void *data);
	void native_Object_keys(const CFunctionsScopePtr &c, void *data);
	void native_Object_getOwnPropertyDescriptor(const CFunctionsScopePtr &c, void *data);
	void native_Object_defineProperty(const CFunctionsScopePtr &c, void *data);
	void native_Object_defineProperties(const CFunctionsScopePtr &c, void *data);

	void native_Object_prototype_hasOwnProperty(const CFunctionsScopePtr &c, void *data);
	void native_Object_prototype_valueOf(const CFunctionsScopePtr &c, void *data);
	void native_Object_prototype_toString(const CFunctionsScopePtr &c, void *data);

	void native_Array(const CFunctionsScopePtr &c, void *data);

	void native_String(const CFunctionsScopePtr &c, void *data);

	void native_RegExp(const CFunctionsScopePtr &c, void *data);

	void native_Number(const CFunctionsScopePtr &c, void *data);

	void native_Boolean(const CFunctionsScopePtr &c, void *data);

	void native_Iterator(const CFunctionsScopePtr &c, void *data);

//	void native_Generator(const CFunctionsScopePtr &c, void *data);
	void native_Generator_prototype_next(const CFunctionsScopePtr &c, void *data);

	void native_Function(const CFunctionsScopePtr &c, void *data);
	void native_Function_prototype_call(const CFunctionsScopePtr &c, void *data);
	void native_Function_prototype_apply(const CFunctionsScopePtr &c, void *data);
	void native_Function_prototype_bind(const CFunctionsScopePtr &c, void *data);
	void native_Function_prototype_isGenerator(const CFunctionsScopePtr &c, void *data);

	void native_Date(const CFunctionsScopePtr &c, void *data);

	void native_Error(const CFunctionsScopePtr &c, void *data);
	void native_EvalError(const CFunctionsScopePtr &c, void *data);
	void native_RangeError(const CFunctionsScopePtr &c, void *data);
	void native_ReferenceError(const CFunctionsScopePtr &c, void *data);
	void native_SyntaxError(const CFunctionsScopePtr &c, void *data);
	void native_TypeError(const CFunctionsScopePtr &c, void *data);


	//////////////////////////////////////////////////////////////////////////
	/// global function

	void native_eval(const CFunctionsScopePtr &c, void *data);
	void native_require(const CFunctionsScopePtr &c, void *data);
	native_require_read_fnc native_require_read;
	void native_isNAN(const CFunctionsScopePtr &c, void *data);
	void native_isFinite(const CFunctionsScopePtr &c, void *data);
	void native_parseInt(const CFunctionsScopePtr &c, void *data);
	void native_parseFloat(const CFunctionsScopePtr &c, void *data);

	void native_JSON_parse(const CFunctionsScopePtr &c, void *data);


	uint32_t uniqueID;
	int32_t currentMarkSlot;
	void *stackBase;
public:
	int32_t getCurrentMarkSlot() const {
		ASSERT(currentMarkSlot >= 0); // UniqueID not allocated
		return currentMarkSlot;
	}
	uint32_t allocUniqueID() {
		++currentMarkSlot;
		ASSERT(currentMarkSlot<TEMPORARY_MARK_SLOTS); // no free MarkSlot
		return ++uniqueID;
	}
	void freeUniqueID() {
		ASSERT(currentMarkSlot >= 0); // freeUniqueID without allocUniqueID
		--currentMarkSlot;
	}
	CScriptVar *first;
	void setTemporaryID_recursive(uint32_t ID);
	void ClearUnreferedVars(const CScriptVarPtr &extra=CScriptVarPtr());
	void setStackBase(void * StackBase) { stackBase = StackBase; }
	void setStackBase(uint32_t StackSize) { char dummy = 0; stackBase = StackSize ? &dummy - StackSize : 0; }
};


//////////////////////////////////////////////////////////////////////////
template<typename T>
inline const CScriptVarPtr &CScriptVar::constScriptVar(T t) { return context->constScriptVar(t); }
//////////////////////////////////////////////////////////////////////////
inline CNumber CScriptVarLink::toNumber() { return var->toNumber(); }
inline CNumber CScriptVarLink::toNumber(CScriptResult &execute) { return var->toNumber(execute); }

inline void CScriptVar::setTemporaryMark(uint32_t ID) { temporaryMark[context->getCurrentMarkSlot()] = ID; }
inline uint32_t CScriptVar::getTemporaryMark() { return temporaryMark[context->getCurrentMarkSlot()]; }


#endif


