/*
 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2025 ardisoft
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

#include <algorithm>
#include "TinyJS.h"

#ifndef NO_REGEXP 
#	if defined HAVE_TR1_REGEX
#		include <tr1/regex>
		using namespace std::tr1;
#	elif defined HAVE_BOOST_REGEX
#		include <boost/regex.hpp>
		using namespace boost;
#	else
#		include <regex>
#	endif
#endif

namespace TinyJS {

 // ----------------------------------------------- Actual Functions

#define CheckObjectCoercible(var) do { \
		if(var->isUndefined() || var->isNull())\
			c->throwError(TypeError, "can't convert undefined to object");\
	}while(0) 

#if PTRDIFF_MAX == INT32_MAX
#	define ptr2int32(p) ((int32_t)p)
#else
#	define ptr2int32(p) ((int32_t)((ptrdiff_t)p) & 0x7FFF)
#endif
static std::string this2string(const CFunctionsScopePtr &c) {
	CScriptVarPtr This = c->getArgument("this");
	CheckObjectCoercible(This);
	return This->toString();
}

static void scStringCharAt(const CFunctionsScopePtr &c, void *) {
	std::string str = this2string(c);
	int p = c->getArgument("pos")->toNumber().toInt32();
	if (p>=0 && p<(int)str.length())
		c->setReturnVar(c->newScriptVar(str.substr(p, 1)));
	else
		c->setReturnVar(c->newScriptVar(""));
}

static void scStringCharCodeAt(const CFunctionsScopePtr &c, void *) {
	std::string str = this2string(c);
	int p = c->getArgument("pos")->toNumber().toInt32();
	if (p>=0 && p<(int)str.length())
		c->setReturnVar(c->newScriptVar((unsigned char)str.at(p)));
	else
		c->setReturnVar(c->constScriptVar(NaN));
}

static void scStringConcat(const CFunctionsScopePtr &c, void *userdata) {
	int length = c->getArgumentsLength();
	std::string str = this2string(c);
	for(int32_t i=ptr2int32(userdata); i<length; i++)
		str.append(c->getArgument(i)->toString());
	c->setReturnVar(c->newScriptVar(str));
}

static void scStringIndexOf(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);
	std::string search = c->getArgument("search")->toString();
	CNumber pos_n = c->getArgument("pos")->toNumber();
	std::string::size_type pos;
	pos = (userdata) ? std::string::npos : 0;
	if(pos_n.sign()<0) pos = 0;
	else if(pos_n.isInfinity()) pos = std::string::npos;
	else if(pos_n.isFinite()) pos = pos_n.toInt32();
	std::string::size_type p = (userdata==0) ? str.find(search, pos) : str.rfind(search, pos);
	if(p==std::string::npos)
		c->setReturnVar(c->newScriptVar(-1));
	else
		c->setReturnVar(c->newScriptVar(p));
}

static void scStringLocaleCompare(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);
	std::string compareString = c->getArgument("compareString")->toString();
	int32_t val = 0;
	if(str<compareString) val = -1;
	else if(str>compareString) val = 1;
	c->setReturnVar(c->newScriptVar(val));
}

static void scStringQuote(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);
	c->setReturnVar(c->newScriptVar(getJSString(str)));
}

#ifndef NO_REGEXP
// helper-function for replace search
static bool regex_search(const std::string &str, const std::string::const_iterator &search_begin, const std::string &substr, bool ignoreCase, bool sticky, std::string::const_iterator &match_begin, std::string::const_iterator &match_end, std::smatch &match) {
	std::regex::flag_type flags = std::regex_constants::ECMAScript;
	if(ignoreCase) flags |= std::regex_constants::icase;
	std::regex_constants::match_flag_type mflag = sticky? std::regex_constants::match_continuous: std::regex_constants::format_default;
	if(str.begin() != search_begin) mflag |= std::regex_constants::match_prev_avail;
	if(regex_search(search_begin, str.end(), match, std::regex(substr, flags), mflag)) {
		match_begin = match[0].first;
		match_end = match[0].second;
		return true;
	}
	return false;
}
static bool regex_search(const std::string &str, const std::string::const_iterator &search_begin, const std::string &substr, bool ignoreCase, bool sticky, std::string::const_iterator &match_begin, std::string::const_iterator &match_end) {
	std::smatch match;
	return regex_search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end, match);
}
#endif /* NO_REGEXP */

static bool charcmp (char i, char j) { return (i==j); }
static bool charicmp (char i, char j) { return (toupper(i)==toupper(j)); }
// helper-function for replace search
static bool string_search(const std::string &str, const std::string::const_iterator &search_begin, const std::string &substr, bool ignoreCase, bool sticky, std::string::const_iterator &match_begin, std::string::const_iterator &match_end) {
	bool (*cmp)(char,char) = ignoreCase ? charicmp : charcmp;
	if(sticky) {
		match_begin = match_end = search_begin;
		std::string::const_iterator s1e=str.end();
		std::string::const_iterator s2=substr.begin(), s2e=substr.end();
		while(match_end!=s1e && s2!=s2e && cmp(*match_end++, *s2++));
		return s2==s2e;
	} 
	match_begin = search(search_begin, str.end(), substr.begin(), substr.end(), cmp);
	if(match_begin==str.end()) return false;
	match_end = match_begin + substr.length();
	return true;
}
//************************************
// Method:    getRegExpData
// FullName:  getRegExpData
// Access:    public static 
// Returns:   bool true if regexp-param=RegExp-Object / other false
// Qualifier:
// Parameter: const CFunctionsScopePtr & c
// Parameter: const string & regexp - parameter name of the regexp
// Parameter: bool noUndefined - true an undefined regexp aims in "" else in "undefined"
// Parameter: const string & flags - parameter name of the flags
// Parameter: string & substr - rgexp.source
// Parameter: bool & global
// Parameter: bool & ignoreCase
// Parameter: bool & sticky
//************************************
static CScriptVarPtr getRegExpData(const CFunctionsScopePtr &c, const std::string &regexp, bool noUndefined, const char *flags_argument, std::string &substr, bool &global, bool &ignoreCase, bool &sticky) {
	CScriptVarPtr regexpVar = c->getArgument(regexp);
	if(regexpVar->isRegExp()) {
#ifndef NO_REGEXP
		CScriptVarRegExpPtr RegExp(regexpVar);
		substr = RegExp->Regexp();
		ignoreCase = RegExp->IgnoreCase();
		global = RegExp->Global();
		sticky = RegExp->Sticky();
		return RegExp;
#endif /* NO_REGEXP */
	} else {
		substr.clear();
		if(!noUndefined || !regexpVar->isUndefined()) substr = regexpVar->toString();
		CScriptVarPtr flagVar;
		if(flags_argument && (flagVar = c->getArgument(flags_argument)) && !flagVar->isUndefined()) {
			std::string flags = flagVar->toString();
			std::string::size_type pos = flags.find_first_not_of("gimy");
			if(pos != std::string::npos) {
				c->throwError(SyntaxError, std::string("invalid regular expression flag ")+flags[pos]);
			}
			global = flags.find_first_of('g')!=std::string::npos;
			ignoreCase = flags.find_first_of('i')!=std::string::npos;
			sticky = flags.find_first_of('y')!=std::string::npos;
		} else
			global = ignoreCase = sticky = false;
	}
	return CScriptVarPtr();
}

static void scStringReplace(const CFunctionsScopePtr &c, void *) {
	const std::string str = this2string(c);
	CScriptVarPtr newsubstrVar = c->getArgument("newsubstr");
	std::string substr, ret_str;
	bool global, ignoreCase, sticky;
	bool isRegExp = (bool)getRegExpData(c, "substr", false, "flags", substr, global, ignoreCase, sticky);
	if(isRegExp && !newsubstrVar->isFunction()) {
#ifndef NO_REGEXP
		std::regex::flag_type flags = std::regex_constants::ECMAScript;
		if(ignoreCase) flags |= std::regex_constants::icase;
		std::regex_constants::match_flag_type mflags = std::regex_constants::match_default;
		if(!global) mflags |= std::regex_constants::format_first_only;
		if(sticky) mflags |= std::regex_constants::match_continuous;
		ret_str = regex_replace(str, std::regex(substr, flags), newsubstrVar->toString(), mflags);
#endif /* NO_REGEXP */
	} else {
		bool (*search)(const std::string &, const std::string::const_iterator &, const std::string &, bool, bool, std::string::const_iterator &, std::string::const_iterator &);
#ifndef NO_REGEXP
		if(isRegExp) 
			search = regex_search;
		else
#endif /* NO_REGEXP */
			search = string_search;
		std::string newsubstr;
		std::vector<CScriptVarPtr> arguments;
		if(!newsubstrVar->isFunction()) 
			newsubstr = newsubstrVar->toString();
		global = global && substr.length();
		std::string::const_iterator search_begin=str.begin(), match_begin, match_end;
		if(search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end)) {
			do {
				ret_str.append(search_begin, match_begin);
				if(newsubstrVar->isFunction()) {
					arguments.push_back(c->newScriptVar(std::string(match_begin, match_end)));
					newsubstr = c->getContext()->callFunction(newsubstrVar, arguments)->toString();
					arguments.pop_back();
				}
				ret_str.append(newsubstr);
#if 1 /* Fix from "vcmpeq" (see Issue 14) currently untested */
				if (match_begin == match_end) {
					if (search_begin != str.end())
						++search_begin;
					else
						break;
				} else {
					search_begin = match_end;
				}
#else
				search_begin = match_end;
#endif
			} while(global && search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end));
		}
		ret_str.append(search_begin, str.end());
	}
	c->setReturnVar(c->newScriptVar(ret_str));
}
#ifndef NO_REGEXP
static void scStringMatch(const CFunctionsScopePtr &c, void *) {
	std::string str = this2string(c);

	std::string flags="flags", substr, newsubstr, match;
	bool global, ignoreCase, sticky;
	CScriptVarRegExpPtr RegExp = getRegExpData(c, "regexp", true, "flags", substr, global, ignoreCase, sticky);
	if(!global) {
		if(!RegExp)
			RegExp = TinyJS::newScriptVar(c->getContext(), substr, flags);
		if(RegExp) {
			try {
				c->setReturnVar(RegExp->exec(str));
			} catch(std::regex_error e) {
				c->throwError(SyntaxError, std::string(e.what())+" - "+CScriptVarRegExp::ErrorStr(e.code()));
			}
		}
	} else {
		try { 
			CScriptVarArrayPtr retVar = c->newScriptVar(Array);
			int idx=0;
			std::string::size_type offset=0;
			global = global && substr.length();
			std::string::const_iterator search_begin=str.begin(), match_begin, match_end;
			if(regex_search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end)) {
				do {
					offset = match_begin-str.begin();
					retVar->addChild(idx++, c->newScriptVar(std::string(match_begin, match_end)));
#if 1 /* Fix from "vcmpeq" (see Issue 14) currently untested */
					if (match_begin == match_end) {
						if (search_begin != str.end())
							++search_begin;
						else
							break;
					} else {
						search_begin = match_end;
					}
#else
					search_begin = match_end;
#endif
				} while(global && regex_search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end));
			}
			if(idx) {
				retVar->addChild("input", c->newScriptVar(str));
				retVar->addChild("index", c->newScriptVar((int)offset));
				c->setReturnVar(retVar);
			} else
				c->setReturnVar(c->constScriptVar(Null));
		} catch(std::regex_error e) {
			c->throwError(SyntaxError, std::string(e.what())+" - "+CScriptVarRegExp::ErrorStr(e.code()));
		}
	}
}
#endif /* NO_REGEXP */

static void scStringSearch(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);

	std::string substr;
	bool global, ignoreCase, sticky;
	getRegExpData(c, "regexp", true, "flags", substr, global, ignoreCase, sticky);
	std::string::const_iterator search_begin=str.begin(), match_begin, match_end;
#ifndef NO_REGEXP
	try { 
		c->setReturnVar(c->newScriptVar(regex_search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end)?match_begin-search_begin:-1));
	} catch(std::regex_error e) {
		c->throwError(SyntaxError, std::string(e.what())+" - "+CScriptVarRegExp::ErrorStr(e.code()));
	}
#else /* NO_REGEXP */
	c->setReturnVar(c->newScriptVar(string_search(str, search_begin, substr, ignoreCase, sticky, match_begin, match_end)?match_begin-search_begin:-1));
#endif /* NO_REGEXP */ 
}

// ----------------------------------------------------------------------
// String.prototype.slice(start, end)
// Signatur: function String.prototype.slice(start, end)
// Extrahiert einen Teilstring, unterstützt negative Indizes (als Slice) und arbeitet über std::string_view.
static void scStringSlice(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);
	int32_t length = c->getArgumentsLength()-(ptr2int32(userdata) & 1);
	bool slice = (ptr2int32(userdata) & 2) == 0;
	int32_t start = c->getArgument("start")->toNumber().toInt32();
	int32_t end = (int32_t)str.size();
	if(slice && start<0) start = (int32_t)(str.size())+start;
	if(length>1) {
		end = c->getArgument("end")->toNumber().toInt32();
		if(slice && end<0) end = (int32_t)(str.size())+end;
	}
	if(!slice && end < start) { end^=start; start^=end; end^=start; }
	if(start<0) start = 0;
	if(start>=(int)str.size()) 
		c->setReturnVar(c->newScriptVar(""));
	else if(end <= start)
		c->setReturnVar(c->newScriptVar(""));
	else {
		std::string_view sv(str);
		auto result_view = sv.substr(start, end - start);
		c->setReturnVar(c->newScriptVar(std::string(result_view)));
	}
}

static void scStringSplit(const CFunctionsScopePtr &c, void *) {
	const std::string str = this2string(c);

	std::string seperator;
	bool global, ignoreCase, sticky;
#ifndef NO_REGEXP
	CScriptVarRegExpPtr RegExp = getRegExpData(c, "separator", true, 0, seperator, global, ignoreCase, sticky);
#else 
	getRegExpData(c, "separator", true, 0, seperator, global, ignoreCase, sticky);
#endif
		
	CScriptVarPtr sep_var = c->getArgument("separator");
	CScriptVarPtr limit_var = c->getArgument("limit");
	int limit = limit_var->isUndefined() ? 0x7fffffff : limit_var->toNumber().toInt32();

	CScriptVarPtr result(c->newScriptVar(Array));
	c->setReturnVar(result);
	if(limit == 0)
		return;
	else if(!str.size() || sep_var->isUndefined()) {
		result->addChild("0", c->newScriptVar(str));
		return;
	}
	if(seperator.size() == 0) {
		for(int i=0; i< std::min((int)str.size(), limit); ++i)
			result->addChild(i, c->newScriptVar(str.substr(i,1)));
		return;
	}
	int length = 0;
	std::string::const_iterator search_begin=str.begin(), match_begin, match_end;
#ifndef NO_REGEXP
	std::smatch match;
#endif
	bool found=true;
	while(found) {
#ifndef NO_REGEXP
		if(RegExp) {
			try { 
				found = regex_search(str, search_begin, seperator, ignoreCase, sticky, match_begin, match_end, match);
			} catch(std::regex_error e) {
				c->throwError(SyntaxError, std::string(e.what())+" - "+CScriptVarRegExp::ErrorStr(e.code()));
			}
		} else /* NO_REGEXP */
#endif
			found = string_search(str, search_begin, seperator, ignoreCase, sticky, match_begin, match_end);
		std::string f;
		if(found) {
			result->addChild(length++, c->newScriptVar(std::string(search_begin, match_begin)));
			if(length>=limit) break;
#ifndef NO_REGEXP
			for(uint32_t i=1; i<match.size(); i++) {
				if(match[i].matched) 
					result->addChild(length++, c->newScriptVar(std::string(match[i].first, match[i].second)));
				else
					result->addChild(length++, c->constScriptVar(Undefined));
				if(length>=limit) break;
			}
			if(length>=limit) break;
#endif
			search_begin = match_end;
		} else {
			result->addChild(length++, c->newScriptVar(std::string(search_begin,str.end())));
			if(length>=limit) break;
		}
	}
}

static void scStringSubstr(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);
	int32_t length = c->getArgumentsLength()-ptr2int32(userdata);
	int32_t start = c->getArgument("start")->toNumber().toInt32();
	if(start<0 || start>=(int)str.size()) 
		c->setReturnVar(c->newScriptVar(""));
	else if(length>1) {
		int length = c->getArgument("length")->toNumber().toInt32();
		c->setReturnVar(c->newScriptVar(str.substr(start, length)));
	} else
		c->setReturnVar(c->newScriptVar(str.substr(start)));
}

static void scStringToLowerCase(const CFunctionsScopePtr &c, void *) {
	std::string str = this2string(c);
	transform(str.begin(), str.end(), str.begin(), ::tolower);
	c->setReturnVar(c->newScriptVar(str));
}

static void scStringToUpperCase(const CFunctionsScopePtr &c, void *) {
	std::string str = this2string(c);
	transform(str.begin(), str.end(), str.begin(), ::toupper);
	c->setReturnVar(c->newScriptVar(str));
}

static void scStringTrim(const CFunctionsScopePtr &c, void *userdata) {
	std::string str = this2string(c);
	std::string::size_type start = 0;
	std::string::size_type end = std::string::npos;
	if(((ptr2int32(userdata)) & 2) == 0) {
		start = str.find_first_not_of(" \t\r\n");
		if(start == std::string::npos) start = 0;
	}
	if(((ptr2int32(userdata)) & 1) == 0) {
		end = str.find_last_not_of(" \t\r\n");
		if(end != std::string::npos) end = 1+end-start;
	}
	c->setReturnVar(c->newScriptVar(str.substr(start, end)));
}



static void scCharToInt(const CFunctionsScopePtr &c, void *) {
	std::string str = c->getArgument("ch")->toString();;
	int val = 0;
	if (str.length()>0)
		val = (int)str.c_str()[0];
	c->setReturnVar(c->newScriptVar(val));
}


static void scStringFromCharCode(const CFunctionsScopePtr &c, void *) {
	char str[2];
	str[0] = c->getArgument("char")->toNumber().toInt32();
	str[1] = 0;
	c->setReturnVar(c->newScriptVar(str));
}

//////////////////////////////////////////////////////////////////////////
// RegExp-Stuff
//////////////////////////////////////////////////////////////////////////

#ifndef NO_REGEXP

static void scRegExpTest(const CFunctionsScopePtr &c, void *) {
	CScriptVarRegExpPtr This = c->getArgument("this");
	if(This)
		c->setReturnVar(This->exec(c->getArgument("str")->toString(), true));
	else
		c->throwError(TypeError, "Object is not a RegExp-Object in test(str)");
}
static void scRegExpExec(const CFunctionsScopePtr &c, void *) {
	CScriptVarRegExpPtr This = c->getArgument("this");
	if(This)
		c->setReturnVar(This->exec(c->getArgument("str")->toString()));
	else
		c->throwError(TypeError, "Object is not a RegExp-Object in exec(str)");
}
#endif /* NO_REGEXP */

// ----------------------------------------------------------------------
// String.prototype.padStart(targetLength, padString)
// Signatur: function String.prototype.padStart(targetLength, padString)
// Falls padString nicht angegeben ist, wird standardmäßig ein einzelnes Leerzeichen genutzt.
static void scStringPadStart(const CFunctionsScopePtr &c, void *) {
    std::string str = this2string(c);
    int targetLength = c->getArgument("targetLength")->toNumber().toInt32();
    std::string padStr = c->getArgument("padString")->isUndefined() ? " " : c->getArgument("padString")->toString();
    if(padStr.empty())
        padStr = " ";
    int currentLength = static_cast<int>(str.size());
    if(targetLength <= currentLength) {
        c->setReturnVar(c->newScriptVar(str));
        return;
    }
    int padLength = targetLength - currentLength;
    std::string padResult;
    while (static_cast<int>(padResult.size()) < padLength) {
        padResult.append(padStr);
    }
    // Nutzung von std::string_view zur Bestimmung des genauen Auffüllbereichs
    std::string_view padView(padResult);
    padView = padView.substr(0, padLength);
    std::string result = std::string(padView) + str;
    c->setReturnVar(c->newScriptVar(result));
}

// ----------------------------------------------------------------------
// String.prototype.padEnd(targetLength, padString)
// Signatur: function String.prototype.padEnd(targetLength, padString)
// Falls padString nicht angegeben ist, wird standardmäßig ein einzelnes Leerzeichen genutzt.
static void scStringPadEnd(const CFunctionsScopePtr &c, void *) {
    std::string str = this2string(c);
    int targetLength = c->getArgument("targetLength")->toNumber().toInt32();
    std::string padStr = c->getArgument("padString")->isUndefined() ? " " : c->getArgument("padString")->toString();
    if(padStr.empty())
        padStr = " ";
    int currentLength = static_cast<int>(str.size());
    if(targetLength <= currentLength) {
        c->setReturnVar(c->newScriptVar(str));
        return;
    }
    int padLength = targetLength - currentLength;
    std::string padResult;
    while (static_cast<int>(padResult.size()) < padLength) {
        padResult.append(padStr);
    }
    std::string_view padView(padResult);
    padView = padView.substr(0, padLength);
    std::string result = str + std::string(padView);
    c->setReturnVar(c->newScriptVar(result));
}

// ----------------------------------------------------------------------
// String.prototype.repeat(count)
// Signatur: function String.prototype.repeat(count)
// Wiederholt den String count-Mal. Bei negativen oder zu großen Werten wird ein Fehler geworfen.
static void scStringRepeat(const CFunctionsScopePtr &c, void *) {
    std::string str = this2string(c);
    int count = c->getArgument("count")->toNumber().toInt32();
    if(count < 0) {
        c->throwError(RangeError, "repeat count must be non-negative");
        return;
    }
    // Begrenze den Wiederholungsfaktor auf einen sinnvollen Maximalwert (z. B. 10.000)
    if(count > 10000) {
        c->throwError(RangeError, "repeat count must not be too large");
        return;
    }
    std::string result;
    result.reserve(str.size() * count);
    for (int i = 0; i < count; i++) {
        result.append(str);
    }
    c->setReturnVar(c->newScriptVar(result));
}

// ----------------------------------------------------------------------
// String.prototype.includes(searchString, position)
// Signatur: function String.prototype.includes(searchString, position)
// Prüft, ob der String ab der gegebenen Position den searchString enthält.
static void scStringIncludes(const CFunctionsScopePtr &c, void *) {
    std::string str = this2string(c);
    std::string searchString = c->getArgument("searchString")->toString();
    int position = c->getArgument("position")->isUndefined() ? 0 : c->getArgument("position")->toNumber().toInt32();
    if (position < 0) position = 0;
    bool found = (str.find(searchString, position) != std::string::npos);
    c->setReturnVar(c->constScriptVar(found));
}

// ----------------------------------------------------------------------
// String.prototype.startsWith(searchString, position)
// Signatur: function String.prototype.startsWith(searchString, position)
// Prüft, ob der String an der angegebenen Position mit searchString beginnt.
static void scStringStartsWith(const CFunctionsScopePtr &c, void *) {
    std::string str = this2string(c);
    std::string searchString = c->getArgument("searchString")->toString();
    int position = c->getArgument("position")->isUndefined() ? 0 : c->getArgument("position")->toNumber().toInt32();
    if (position < 0) position = 0;
    bool starts = false;
    std::string_view sv(str);
    if (static_cast<int>(sv.size()) >= position + static_cast<int>(searchString.size())) {
        auto view = sv.substr(position, searchString.size());
        starts = (view == searchString);
    }
    c->setReturnVar(c->constScriptVar(starts));
}

// ----------------------------------------------------------------------
// String.prototype.endsWith(searchString, length)
// Signatur: function String.prototype.endsWith(searchString, length)
// Prüft, ob der String (bis zur optionalen Länge) mit searchString endet.
static void scStringEndsWith(const CFunctionsScopePtr &c, void *) {
    std::string str = this2string(c);
    std::string searchString = c->getArgument("searchString")->toString();
    int length = c->getArgument("length")->isUndefined() ? static_cast<int>(str.size()) : c->getArgument("length")->toNumber().toInt32();
    if (length > static_cast<int>(str.size())) length = static_cast<int>(str.size());
    bool ends = false;
    if (length >= static_cast<int>(searchString.size())) {
        std::string_view sv(str);
        auto view = sv.substr(length - searchString.size(), searchString.size());
        ends = (view == searchString);
    }
    c->setReturnVar(c->constScriptVar(ends));
}

#if 0
// ----------------------------------------------------------------------
// String.prototype.substr(start, length)
// Signatur: function String.prototype.substr(start, length)
// Extrahiert einen Teilstring beginnend bei start, maximal length Zeichen.
static void scStringSubstr(const CFunctionsScopePtr &c, void *userdata) {
    std::string str = this2string(c);
    int32_t countParam = c->getArgumentsLength() - ptr2int32(userdata);
    int32_t start = c->getArgument("start")->toNumber().toInt32();
    if(start < 0 || start >= (int)str.size()) {
        c->setReturnVar(c->newScriptVar(""));
    } else if(countParam > 1) {
        int len = c->getArgument("length")->toNumber().toInt32();
        std::string_view sv(str);
        auto result_view = sv.substr(start, len);
        c->setReturnVar(c->newScriptVar(std::string(result_view)));
    } else {
        std::string_view sv(str);
        auto result_view = sv.substr(start);
        c->setReturnVar(c->newScriptVar(std::string(result_view)));
    }
}
#endif
// ----------------------------------------------- Register Functions
void registerStringFunctions(CTinyJS *tinyJS) {}
extern "C" void _registerStringFunctions(CTinyJS *tinyJS) {
	CScriptVarPtr fnc;
	// charAt
	tinyJS->addNative("function String.prototype.charAt(pos)", scStringCharAt, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.charAt(this,pos)", scStringCharAt, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// charCodeAt
	tinyJS->addNative("function String.prototype.charCodeAt(pos)", scStringCharCodeAt, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.charCodeAt(this,pos)", scStringCharCodeAt, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// concat
	tinyJS->addNative("function String.prototype.concat()", scStringConcat, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.concat(this)", scStringConcat, (void*)1, SCRIPTVARLINK_BUILDINDEFAULT);
	// indexOf
	tinyJS->addNative("function String.prototype.indexOf(search,pos)", scStringIndexOf, 0, SCRIPTVARLINK_BUILDINDEFAULT); // find the position of a string in a string, -1 if not
	tinyJS->addNative("function String.indexOf(this,search,pos)", scStringIndexOf, 0, SCRIPTVARLINK_BUILDINDEFAULT); // find the position of a string in a string, -1 if not
	// lastIndexOf
	tinyJS->addNative("function String.prototype.lastIndexOf(search,pos)", scStringIndexOf, (void*)-1, SCRIPTVARLINK_BUILDINDEFAULT); // find the last position of a string in a string, -1 if not
	tinyJS->addNative("function String.lastIndexOf(this,search,pos)", scStringIndexOf, (void*)-1, SCRIPTVARLINK_BUILDINDEFAULT); // find the last position of a string in a string, -1 if not
	// localeCompare
	tinyJS->addNative("function String.prototype.localeCompare(compareString)", scStringLocaleCompare, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.localeCompare(this,compareString)", scStringLocaleCompare, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// quote
	tinyJS->addNative("function String.prototype.quote()", scStringQuote, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.quote(this)", scStringQuote, 0, SCRIPTVARLINK_BUILDINDEFAULT);
#ifndef NO_REGEXP
	// match
	tinyJS->addNative("function String.prototype.match(regexp, flags)", scStringMatch, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.match(this, regexp, flags)", scStringMatch, 0, SCRIPTVARLINK_BUILDINDEFAULT);
#endif /* !REGEXP */
	// replace
	tinyJS->addNative("function String.prototype.replace(substr, newsubstr, flags)", scStringReplace, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.replace(this, substr, newsubstr, flags)", scStringReplace, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// search
	tinyJS->addNative("function String.prototype.search(regexp, flags)", scStringSearch, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.search(this, regexp, flags)", scStringSearch, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// slice
	tinyJS->addNative("function String.prototype.slice(start,end)", scStringSlice, 0, SCRIPTVARLINK_BUILDINDEFAULT); // find the last position of a string in a string, -1 if not
	tinyJS->addNative("function String.slice(this,start,end)", scStringSlice, (void*)1, SCRIPTVARLINK_BUILDINDEFAULT); // find the last position of a string in a string, -1 if not
	// split
	tinyJS->addNative("function String.prototype.split(separator,limit)", scStringSplit, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.split(this,separator,limit)", scStringSplit, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// substr
	tinyJS->addNative("function String.prototype.substr(start,length)", scStringSubstr, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.substr(this,start,length)", scStringSubstr, (void*)1, SCRIPTVARLINK_BUILDINDEFAULT);
	// substring
	tinyJS->addNative("function String.prototype.substring(start,end)", scStringSlice, (void*)2, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.substring(this,start,end)", scStringSlice, (void*)3, SCRIPTVARLINK_BUILDINDEFAULT);
	// toLowerCase toLocaleLowerCase currently the same function
	tinyJS->addNative("function String.prototype.toLowerCase()", scStringToLowerCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.toLowerCase(this)", scStringToLowerCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.toLocaleLowerCase()", scStringToLowerCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.toLocaleLowerCase(this)", scStringToLowerCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// toUpperCase toLocaleUpperCase currently the same function
	tinyJS->addNative("function String.prototype.toUpperCase()", scStringToUpperCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.toUpperCase(this)", scStringToUpperCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.toLocaleUpperCase()", scStringToUpperCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.toLocaleUpperCase(this)", scStringToUpperCase, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// trim
	tinyJS->addNative("function String.prototype.trim()", scStringTrim, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.trim(this)", scStringTrim, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	// trimLeft
	tinyJS->addNative("function String.prototype.trimLeft()", scStringTrim, (void*)1, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.trimLeft(this)", scStringTrim, (void*)1, SCRIPTVARLINK_BUILDINDEFAULT);
	// trimRight
	tinyJS->addNative("function String.prototype.trimRight()", scStringTrim, (void*)2, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.trimRight(this)", scStringTrim, (void*)2, SCRIPTVARLINK_BUILDINDEFAULT);

	tinyJS->addNative("function charToInt(ch)", scCharToInt, 0, SCRIPTVARLINK_BUILDINDEFAULT); //  convert a character to an int - get its value
	
	tinyJS->addNative("function String.fromCharCode(char)", scStringFromCharCode, 0, SCRIPTVARLINK_BUILDINDEFAULT);
#ifndef NO_REGEXP
	tinyJS->addNative("function RegExp.prototype.test(str)", scRegExpTest, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function RegExp.prototype.exec(str)", scRegExpExec, 0, SCRIPTVARLINK_BUILDINDEFAULT);
#endif /* NO_REGEXP */


	tinyJS->addNative("function String.prototype.padStart(targetLength, padString)", scStringPadStart, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.padStart(this, targetLength, padString)", scStringPadStart, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.padEnd(targetLength, padString)", scStringPadEnd, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.padEnd(this, targetLength, padString)", scStringPadEnd, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.repeat(count)", scStringRepeat, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.repeat(this, count)", scStringRepeat, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.includes(searchString, position)", scStringIncludes, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.includes(this, searchString, position)", scStringIncludes, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.startsWith(searchString, position)", scStringStartsWith, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.startsWith(this, searchString, position)", scStringStartsWith, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.prototype.endsWith(searchString, length)", scStringEndsWith, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function String.endsWith(this, searchString, length)", scStringEndsWith, 0, SCRIPTVARLINK_BUILDINDEFAULT);
}

} /* namespace TinyJS */