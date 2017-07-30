#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

class analyzer {
public:
	enum MsgType { ERR_SYN, ERR_IN, ERR_OUT, INFO_FIN, INFO_HELP };

private:
	struct report {
		unsigned int totalLine;
		unsigned int codeLine;
		unsigned int commentLine;
		unsigned int blankLine;
		unsigned int funcCount;
	};

	struct funcdata {
		string name;
		unsigned int line;
		unsigned int start;
	};

	struct macrodata {
		string name;
		vector<string> param;
		string macro;
	};

	enum StatusCode { SC_NORMAL, SC_STRING, SC_COMMENT, SC_LINECOMMENT, \
		SC_STRUCT_BEGIN, SC_STRUCT_ID, SC_STRUCT_COMPLETE, \
		SC_UNION_BEGIN, SC_UNION_ID, SC_UNION_COMPLETE, \
		SC_ENUM_BEGIN, SC_ENUM_ID, SC_ENUM_COMPLETE, \
		SC_TYPEDEF_BEGIN, SC_TYPEDEF_SPEC, SC_TYPEDEF_REP, SC_TYPEDEF_ID, \
		SC_SHARP_BEGIN, SC_DEFINE_BEGIN, SC_DEFINE_ID, SC_DEFINE_PAR, SC_DEFINE_MACRO, \
		SC_FUNC_BEGIN, SC_FUNC_ID, SC_FUNC_WAIT };

	enum TypeCode { TC_EMPTY, TC_LPAR, TC_RPAR, TC_LBRACE, TC_RBRACE, TC_SHARP, TC_COMMA, TC_SEMI, TC_QUOTE, \
		TC_COM, TC_LCOM, TC_RCOM, TC_KEYWORD, TC_STRUCT, TC_UNION, TC_ENUM, TC_TYPEDEF, TC_TYPE, TC_ID, TC_OTHER };

	enum SpecType { ST_STRUCT, ST_UNION, ST_ENUM, ST_NORMAL };

	bool extOutput;
	string outputPath;
	string inputPath;
	report result;
	void(*pInfo)(MsgType);

	StatusCode status, statusCache;
	bool isComment;
	bool lineComment;
	map<string, macrodata> macroInfo;
	vector<funcdata> funcInfo;
	string identifier;
	int braceLayer;
	int parentheseLayer;
	const string basicType[9] = { "void", "char", "short", "int", "long", "float", "double", "signed", "unsigned" };
	const int basicTypeCount = 9;
	const string keyword[19] = { "auto", "break", "case", "const", "continue", "default", "do", "else", "extern", \
		"for", "goto", "if", "register", "return", "sizeof", "static", "switch", "volatile", "while" };
	const int keywordCount = 19;
	vector<string> typeList;
	vector<string> specList[3]; // 0-struct 1-union 2-enum

	void get_ready();
	void macro_replace(string &buf);
	void process_line(string &buf);
	void process_word(const string &word);
	string read_word(string &s, unsigned int &pos);
	void replace(string &s, const string &s1, const string &s2);
	void format_line(string &s);
	TypeCode word_type(const string &word);
	bool is_indentifier(const string &word);
	void change_status(const StatusCode &next);
	void set_identifier(const string &str);
	void add_type(const SpecType &type);
	bool find_spec(const SpecType &type, const string &str);
	double average_length();
	string grade(int mode);
	funcdata longest();

public:
	analyzer();
	~analyzer() { };
	void init(string _input, string _output, bool _ext, void(*_p)(MsgType));
	void analyze();
	void output();

};

void DisplayMsg(analyzer::MsgType);

int main(int argc, char *argv[]) {
	bool extOutput = false;
	if (argc < 2) {
		DisplayMsg(analyzer::ERR_SYN);
		exit(1);
	}
	if (strcmp(argv[1], "/?") == 0) {
		DisplayMsg(analyzer::INFO_HELP);
		exit(0);
	}

	string inputPath(argv[1]), outputPath;
	if (argc > 2) {
		if (strcmp(argv[2], "/o") == 0 && argc > 3) {
			extOutput = true;
			outputPath = argv[3];
		}
		else {
			DisplayMsg(analyzer::ERR_SYN);
			exit(1);
		}
	}

	analyzer a;
	a.init(inputPath, outputPath, extOutput, DisplayMsg);
	a.analyze();
	a.output();

	return 0;
}

analyzer::analyzer() {
	extOutput = false;
	outputPath = "", inputPath = "";
	memset(&result, 0, sizeof(result));
	pInfo = NULL;
}

void analyzer::init(string _input, string _output, bool _ext, void(*_p)(MsgType)) {
	if (_ext) {
		extOutput = true;
		outputPath = _output;
	}
	inputPath = _input;
	pInfo = _p;
}

void analyzer::analyze() {
	get_ready();

	ifstream in(inputPath, ios::in);
	if (!in.is_open()) {
		(*pInfo)(ERR_IN);
		exit(1);
	}

	while (!in.eof()) {
		string str;
		while (!in.eof()) {
			string buf;
			getline(in, buf);
			format_line(buf);
			str += buf;
			if (buf.length() == 0 || buf[buf.length() - 1] != '\\')
				break;
			str.erase(--str.end());
		};
		process_line(str);
	}

	// Todo
}

void analyzer::output() {
	if (extOutput) {
		if (outputPath == "") {
			(*pInfo)(ERR_OUT);
			exit(1);
		}
		ofstream out;
		out.open(outputPath, ios::out | ios::trunc);
		if (!out.is_open()) {
			(*pInfo)(ERR_OUT);
			exit(1);
		}
		out << "The result of analyzing program file \" " << inputPath << "\":" << endl << endl;
		out << "\t" << "Lines of code:" << "\t\t" << result.codeLine << " (" << result.codeLine * 100 / result.totalLine << "%)" << endl;
		out << "\t" << "Lines of comments:" << "\t" << result.commentLine << " (" << result.commentLine * 100 / result.totalLine << "%)" << endl;
		out << "\t" << "Blank lines:" << "\t\t" << result.blankLine << " (" << result.blankLine * 100 / result.totalLine << "%)" << endl << endl;
		out << "\t" << "The program includes " << result.funcCount << " functions." << endl;
		out << "\t" << "The average length of a section of code is " << setprecision(2) << average_length() << " lines." << endl;
		if (!funcInfo.empty()) {
			funcdata l = longest();
			out << "\t" << "The longest function is \"" << l.name << "\", of which length is " << l.line << ", position is " << l.start << "." << endl << endl;
		}
		out << "\t" << grade(0) << endl;
		out << "\t" << grade(1) << endl;
		out << "\t" << grade(2) << endl;
		out.close();
	}
	else {
		cout << endl;
		cout << "The result of analyzing program file \" " << inputPath << "\":" << endl << endl;
		cout << "\t" << "Lines of code:" << "\t\t" << result.codeLine << " (" << result.codeLine * 100 / result.totalLine << "%)" << endl;
		cout << "\t" << "Lines of comments:" << "\t" << result.commentLine << " (" << result.commentLine * 100 / result.totalLine << "%)" << endl;
		cout << "\t" << "Blank lines:" << "\t\t" << result.blankLine << " (" << result.blankLine * 100 / result.totalLine << "%)" << endl << endl;
		cout << "\t" << "The program includes " << result.funcCount << " functions." << endl;
		cout << "\t" << "The average length of a section of code is " << setprecision(2) << average_length() << " lines." << endl;
		if (!funcInfo.empty()) {
			funcdata l = longest();
			cout << "\t" << "The longest function is \"" << l.name << "\", of which length is " << l.line << ", position is " << l.start << "." << endl << endl;
		}
		cout << "\t" << grade(0) << endl;
		cout << "\t" << grade(1) << endl;
		cout << "\t" << grade(2) << endl;
	}
}

void analyzer::get_ready() {
	memset(&result, 0, sizeof(result));
	status = statusCache = SC_NORMAL;
	braceLayer = 0, parentheseLayer = 0, identifier = "";
	isComment = false, lineComment = false;
	funcInfo.clear();
	typeList.clear();
	for (int i = 0; i < basicTypeCount; i++)
		typeList.push_back(basicType[i]);
	for (int i = 0; i < 3; i++)
		specList[i].clear();
}

void analyzer::macro_replace(string &buf) {
	string word;
	unsigned int pos = 0;
	while (pos < buf.length()) {
		int begin = pos;
		word = (read_word(buf, pos));
		if (macroInfo.find(word) != macroInfo.end()) {
			macrodata m = macroInfo[word];
			string rep = m.macro;
			if (m.param.size() > 0)
				word = (read_word(buf, pos));
			for (unsigned int i = 0; i < m.param.size(); i++) {
				string param;
				while (true) {
					word = (read_word(buf, pos));
					if (word == "," || word == ")")
						break;
					param += (" " + word);
				}
				replace(rep, m.param[i], param);
			}
			int end = pos - 1;
			buf.erase(begin, end - begin);
			buf.insert(begin, rep);
			pos += (rep.length() - (end - begin));
		}
	}
	format_line(buf);
}

void analyzer::process_line(string &buf) {
	result.totalLine++;
	if (braceLayer > 0)
		funcInfo[result.funcCount].line++;
	if (status == SC_DEFINE_MACRO)
		status = SC_NORMAL;
	if (lineComment) {
		lineComment = false;
		status = statusCache;
	}
	macro_replace(buf);

	if (buf.length() == 0) {
		result.blankLine++;
		return;
	}

	if (buf.length() > 1 && buf.substr(0, 2) == "//") {
		result.commentLine++;
		return;
	}

	if (!isComment)
		result.codeLine++;

	unsigned int pos = 0;
	while (pos < buf.length())
		process_word(read_word(buf, pos));

	if (isComment || lineComment)
		result.commentLine++;
	// Todo
}

void analyzer::process_word(const string &word) {
	TypeCode type = word_type(word);

	if (status == SC_LINECOMMENT && !lineComment)
		change_status(SC_NORMAL);

	if (status == SC_DEFINE_MACRO) {
		macroInfo[identifier].macro += (word + " ");
		return;
	}

	if (type == TC_COM) {
		statusCache = status;
		lineComment = true;
		change_status(SC_LINECOMMENT);
	}

	if (status != SC_COMMENT && type == TC_LCOM) {
		statusCache = status;
		isComment = true;
		change_status(SC_COMMENT);
	}

	if (type == TC_QUOTE) {
		statusCache = status;
		change_status(SC_STRING);
	}

	switch (status) {
	case SC_NORMAL: {
		switch (type) {
		case TC_STRUCT: change_status(SC_STRUCT_BEGIN); break;
		case TC_UNION: change_status(SC_UNION_BEGIN); break;
		case TC_ENUM: change_status(SC_ENUM_BEGIN); break;
		case TC_TYPEDEF: change_status(SC_TYPEDEF_BEGIN); break;
		case TC_TYPE: change_status(SC_FUNC_BEGIN); break;
		case TC_SHARP: change_status(SC_SHARP_BEGIN); break;
		}
		break;
	}
	case SC_STRUCT_BEGIN: {
		switch (type) {
		case TC_ID: if (find_spec(ST_STRUCT, word)) change_status(SC_FUNC_BEGIN); else { set_identifier( word); change_status(SC_STRUCT_ID); } break;
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; break;
		}
		break;
	}
	case SC_STRUCT_ID: {
		switch (type) {
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; change_status(SC_STRUCT_COMPLETE); break;
		}
		break;
	}
	case SC_STRUCT_COMPLETE: {
		switch (type) {
		case TC_SEMI: add_type(ST_STRUCT); break;
		}
		break;
	}
	case SC_UNION_BEGIN: {
		switch (type) {
		case TC_ID: if (find_spec(ST_UNION, word)) change_status(SC_FUNC_BEGIN); else { set_identifier(word); change_status(SC_UNION_ID); } break;
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; break;
		}
		break;
	}
	case SC_UNION_ID: {
		switch (type) {
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; change_status(SC_UNION_COMPLETE); break;
		}
		break;
	}
	case SC_UNION_COMPLETE: {
		switch (type) {
		case TC_SEMI: add_type(ST_UNION); break;
		}
		break;
	}
	case SC_ENUM_BEGIN: {
		switch (type) {
		case TC_ID: if (find_spec(ST_ENUM, word)) change_status(SC_FUNC_BEGIN); else { set_identifier(word); change_status(SC_ENUM_ID); } break;
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; break;
		}
		break;
	}
	case SC_ENUM_ID: {
		switch (type) {
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; change_status(SC_ENUM_COMPLETE); break;
		}
		break;
	}
	case SC_ENUM_COMPLETE: {
		switch (type) {
		case TC_SEMI: add_type(ST_ENUM); break;
		}
		break;
	}
	case SC_TYPEDEF_BEGIN: {
		switch (type) {
		case TC_TYPE: change_status(SC_TYPEDEF_REP); break;
		case TC_STRUCT: 
		case TC_UNION: 
		case TC_ENUM: change_status(SC_TYPEDEF_SPEC); break;
		}
		break;
	}
	case SC_TYPEDEF_SPEC: {
		switch (type) {
		case TC_ID: change_status(SC_TYPEDEF_REP); break;
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--; change_status(SC_TYPEDEF_REP); break;
		}
		break;
	}
	case SC_TYPEDEF_REP: {
		switch (type) {
		case TC_ID: set_identifier(word); change_status(SC_TYPEDEF_ID); break;
		}
		break;
	}
	case SC_TYPEDEF_ID: {
		switch (type) {
		case TC_SEMI: add_type(ST_NORMAL); break;
		}
		break;
	}
	case SC_FUNC_BEGIN: {
		switch (type) {
		case TC_ID: set_identifier(word); change_status(SC_FUNC_ID); break;
		}
		break;
	}
	case SC_FUNC_ID: {
		switch (type) {
		case TC_LPAR: parentheseLayer++; break;
		case TC_RPAR: parentheseLayer--; 
			if (parentheseLayer == 0) {
				funcInfo.push_back({ identifier, 1u, result.totalLine });
				change_status(SC_FUNC_WAIT);
			}
			break;
		}
		break;
	}
	case SC_FUNC_WAIT: {
		switch (type) {
		case TC_LBRACE: braceLayer++; break;
		case TC_RBRACE: braceLayer--;
			if (braceLayer == 0) {
				result.funcCount++;
				change_status(SC_NORMAL);
			}
			break;
		}
		break;
	}
	case SC_COMMENT: {
		switch (type) {
		case TC_RCOM: isComment = false; result.commentLine++; change_status(statusCache); break;
		}
		break;
	}
	case SC_SHARP_BEGIN: {
		switch (type) {
		case TC_ID: if (word == "define") change_status(SC_DEFINE_BEGIN); else change_status(SC_NORMAL); break;
		}
		break;
	}
	case SC_DEFINE_BEGIN: {
		switch (type) {
		case TC_ID: {
			set_identifier(word);
			macrodata t;
			t.name = identifier;
			macroInfo[identifier] = t;
			change_status(SC_DEFINE_ID);
			break;
		}
		}
		break;
	}
	case SC_DEFINE_ID: {
		switch (type) {
		case TC_ID: change_status(SC_DEFINE_MACRO); break;
		case TC_LPAR: change_status(SC_DEFINE_PAR); break;
		}
		break;
	}
	case SC_DEFINE_PAR: {
		switch (type) {
		case TC_ID: macroInfo[identifier].param.push_back(word); break;
		case TC_COMMA: break;
		case TC_RPAR: change_status(SC_DEFINE_MACRO); break;
		}
		break;
	}
	case SC_STRING: {
		switch (type) {
		case TC_QUOTE: change_status(SC_NORMAL); break;
		}
	}
	}

	if (type == TC_SEMI)
		change_status(SC_NORMAL);

}

string analyzer::read_word(string &s, unsigned int &pos) {
	string re;
	if (s.length() == 0)
		return re;
	int blank = s.find(" ", pos);
	if (blank == string::npos)
		blank = s.length();
	re = s.substr(pos, blank - pos);
	pos = blank + 1;
	return re;
}

void analyzer::replace(string &s, const string &s1, const string &s2) {
	int offset = s2.rfind(s1);
	if (offset == string::npos)
		offset = -1;

	int found = s.find(s1);
	while (found != string::npos) {
		s.replace(found, s1.length(), s2);
		found = s.find(s1, found + offset + 1);
	}
}

void analyzer::format_line(string &s) {
	if (s.empty())
		return;

	const string repList = "(){}[]<>#*,;\"";
	for (unsigned int i = 0; i < repList.length(); i++) {
		string s1 = repList.substr(i, 1);
		replace(s, s1, " " + s1 + " ");
	}
	replace(s, "//", " // ");
	replace(s, "/ *", " /* ");
	replace(s, "* /", " */ ");
	replace(s, "\t", " ");
	replace(s, "  ", " ");

	s.erase(0, s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
}

analyzer::TypeCode analyzer::word_type(const string &word) {
	if (word.length() == 0)
		return TC_EMPTY;

	if (word=="(")
		return TC_LPAR;

	if (word == ")")
		return TC_RPAR;

	if (word == "{")
		return TC_LBRACE;

	if (word == "}")
		return TC_RBRACE;

	if (word == "#")
		return TC_SHARP;

	if (word == ",")
		return TC_COMMA;

	if (word == ";")
		return TC_SEMI;

	if (word == "\"")
		return TC_QUOTE;

	if (word == "//")
		return TC_COM;

	if (word == "/*")
		return TC_LCOM;

	if (word == "*/")
		return TC_RCOM;

	for (int i = 0; i < keywordCount; i++)
		if (word == keyword[i])
			return TC_KEYWORD;

	if (word == "struct")
		return TC_STRUCT;

	if (word == "union")
		return TC_UNION;

	if (word == "enum")
		return TC_ENUM;

	if (word == "typedef")
		return TC_TYPEDEF;

	for (unsigned int i = 0; i < typeList.size(); i++)
		if (word == typeList[i])
			return TC_TYPE;

	if (is_indentifier(word))
		return TC_ID;

	return TC_OTHER;
}

bool analyzer::is_indentifier(const string &word) {
	if (word.length() == 0)
		return false;
	if (word[0] == '_' || (word[0] >= 'A' && word[0] <= 'Z') || (word[0] >= 'a' && word[0] <= 'z')) {
		for (unsigned int i = 1; i < word.length(); i++)
			if (word[i] == '_' || (word[i] >= 'A' && word[i] <= 'Z') || (word[i] >= 'a' && word[i] <= 'z') || (word[i] >= '0' && word[i] <= '9'))
				continue;
			else
				return false;
		return true;
	}
	else {
		return false;
	}
}

void analyzer::change_status(const StatusCode &next) {
	if (braceLayer == 0 && parentheseLayer == 0)
		status = next;
}

void analyzer::set_identifier(const string &str) {
	if (braceLayer == 0 && parentheseLayer == 0)
		identifier = str;
}

void analyzer::add_type(const SpecType &type) {
	if (type == ST_NORMAL)
		typeList.push_back(identifier);
	else
		specList[(unsigned int)type].push_back(identifier);
}

bool analyzer::find_spec(const SpecType &type, const string &str) {
	if (type == ST_NORMAL)
		return false;
	if (find(specList[(unsigned int)type].begin(), specList[(unsigned int)type].end(), str) != specList[(unsigned int)type].end())
		return true;
	else
		return false;
}

double analyzer::average_length() {
	if (result.funcCount == 0)
		return 0;
	int sum = 0;
	for (unsigned int i = 0; i < result.funcCount; i++)
		sum += funcInfo[i].line;
	return (double)sum / result.funcCount;
}

string analyzer::grade(int mode) {
	if (mode == 0) {
		double ave = average_length();
		if (ave >= 10 && ave <= 15)
			return "Grade A: Excellent routine size style.";
		else if ((ave >= 8 && ave <= 9) || (ave >= 16 && ave <= 20))
			return "Grade B: Good routine size style.";
		else if ((ave >= 5 && ave <= 7) || (ave >= 21 && ave <= 24))
			return "Grade C: Not bad routine size style.";
		else if (ave < 5 || ave > 24)
			return "Grade D: Bad routine size style.";
	}

	if (mode == 1 || mode==2) {
		int ratio;
		string word;
		if (mode == 1)
			ratio = result.commentLine * 100 / result.totalLine, word = "commenting";
		else
			ratio = result.blankLine * 100 / result.totalLine, word = "white space";

		if (ratio >= 15 && ratio <= 25)
			return "Grade A: Excellent " + word + " style.";
		else if ((ratio >= 10 && ratio <= 14) || (ratio >= 26 && ratio <= 30))
			return "Grade B: Good " + word + " style.";
		else if ((ratio >= 5 && ratio <= 9) || (ratio >= 31 && ratio <= 35))
			return "Grade C: Not bad " + word + " style.";
		else if (ratio < 5 || ratio > 35)
			return "Grade D: Bad " + word + " style.";
	}

	return "";
}

analyzer::funcdata analyzer::longest() {
	if (funcInfo.empty())
		return { "null", 0, 0 };
	unsigned int m = 0;
	for (unsigned int i = 1; i < funcInfo.size(); i++)
		if (funcInfo[i].line > m)
			m = i;
	return funcInfo[m];
}

void DisplayMsg(analyzer::MsgType type) {
	switch (type) {
	case analyzer::ERR_SYN:
		cout << "Error: Invalid syntax." << endl\
			<< "To get help, input \"analyzer /?\"." << endl;
		break;

	case analyzer::ERR_IN:
		cout << "Error: Cannot read source file." << endl;
		break;

	case analyzer::ERR_OUT:
		cout << "Error: Cannot access output file." << endl;
		break;

	case analyzer::INFO_FIN:
		cout << "Info: Finish Analysis." << endl;
		break;

	case analyzer::INFO_HELP:
		cout << endl\
			<< "Syntax:  analyzer input_file [/o output_file]" << endl << endl\
			<< "Description:  Analyzes the style of a C-programming source file and outputs a report." << endl << endl\
			<< "Parameters: " << endl\
			<< "\t/o output_file : Outputs the report into a disk file instead of displaying at the command prompt." << endl\
			<< "\t/? : Displays help at the command prompt." << endl << endl\
			<< "Examples: " << endl\
			<< "\tanalyzer example.c" << endl\
			<< "\tanalyzer example.c /o report.txt" << endl;
		break;

	}
}
