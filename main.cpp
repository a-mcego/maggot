#include "Global.h"
#include <regex>

vector<string> GetLines(string filename)
{
	ifstream file(filename);
	vector<string> ret;
	while (true)
	{
		string str;
		std::getline(file, str);
		ret.push_back(str);
		if (file.eof())
			break;
	}
	return ret;
}

vector<vector<string>> GetVectorLines(string filename)
{
    vector<vector<string>> ret;

    auto lines = GetLines(filename);
    ret.push_back(vector<string>());
    for(auto& line: lines)
    {
        if (line.empty())
            ret.push_back(vector<string>());
        else
            ret.back().push_back(line);
    }
    while (ret.back().empty())
        ret.pop_back();
    return ret;
}

string Match(string str, string reg)
{
    std::regex reg_obj(reg);
    std::smatch matches;
    std::regex_search(str, matches, reg_obj);
    return matches.str();
}

string LineWhitespace(string str)
{
    return Match(str, "^\\s+");
}

void PrintNums(string s)
{
    for(auto& c: s)
    {
        cout << int((unsigned char)(c)) << " ";
    }
    cout << endl;
}

bool HasCommonPrefix(string s1, string s2)
{
    size_t minlen = std::min(s1.size(), s2.size());
    return s1.substr(0,minlen) == s2.substr(0,minlen);
}

struct TokenType
{
    string name, regexp;
};

vector<TokenType> MakeTokens(vector<vector<string>> vv)
{
    vector<TokenType> ret;
    for(auto& v: vv)
    {
        if(v.size() != 2)
            KILL("TokenType definitions wrong.");
        ret.push_back(TokenType{v[0],v[1]});
    }
    return ret;
}

vector<Token> Tokenize(const string& source, const string& grammar)
{
    vector<TokenType> token_regexes = MakeTokens(GetVectorLines(grammar));
	vector<Token> ret;

	size_t position = 0;
	while (position < source.length())
	{
		string curr_str = source.substr(position);

		vector<Token> found_tokens;

		for (const auto& token_regex : token_regexes)
		{
			std::regex reg_obj("^" + token_regex.regexp);
			std::smatch matches;

			std::regex_search(curr_str, matches, reg_obj);

			Token new_token{ token_regex.name, matches.str() };

			if (new_token.data.empty())
                continue;

			if (found_tokens.empty())
			{
				found_tokens.push_back(new_token);
			}
			else
			{
				if (found_tokens[0].data.length() < matches.length())
				{
					found_tokens.clear();
					found_tokens.push_back(new_token);
				}
				else if (found_tokens[0].data.length() == matches.length())
				{
					found_tokens.push_back(new_token);
				}
			}
		}

		if (found_tokens.empty())
		{
			cout << "Couldn't find tokens at position " << position << endl;
			cout << source.substr(position);
			return {};
		}

		auto& found_token = found_tokens[0];
		if (found_token.type[0] != '!') //ignore tokens with !
            ret.push_back(found_token);
		position += found_token.data.length();
	}
	return ret;
}

#include "Parser.h"
#include "Runner.h"
#include "Bracer.h"

void Main(const vector<string>& args)
{
    if (args.size() != 2)
        KILL("Give .mag file as param.");

    auto lines = GetLines(args[1]);

    //Remove comments
    for(auto& line: lines)
        line = line.substr(0,line.find("//"));

    //Remove empty lines
    for(size_t i=0; i<lines.size();)
    {
        auto& line = lines[i];
        if (Match(line, "^\\s+$").length() > 0 or line.empty())
            lines.erase(lines.begin()+i);
        else
            ++i;
    }
    lines.push_back("");

    Bracer::AddBraces(lines);

    string totalcode;
    for(auto& line: lines)
    {
        totalcode += line;
        totalcode += "\n";
    }

    auto LoadGrammar = [](string name)->string
    {
        auto lines = GetLines(name);
        lines.push_back("");
        //Remove empty lines
        for(size_t i=0; i<lines.size();)
        {
            auto& line = lines[i];
            if (Match(line, "^\\s+$").length() > 0 or line.empty())
                lines.erase(lines.begin()+i);
            else
                ++i;
        }
        string ret;
        for(auto& line: lines)
        {
            ret += line;
            ret += "\n";
        }
        return ret;
    };

    {
        auto grammargrammar_tokens = Tokenize(LoadGrammar("grammar.grm"),"grammar.tkn");
        Grammar p;
        p.LoadDefaultGrammar();
        p.Parse(grammargrammar_tokens);

        auto maggotgrammar_tokens = Tokenize(LoadGrammar("maggot.grm"),"grammar.tkn");
        Grammar p2;
        p2.LoadGrammarFromAST(p.ast);
        p2.Parse(maggotgrammar_tokens);

        Grammar p3;
        p3.LoadGrammarFromAST(p2.ast);
        auto maggot_code = Tokenize(totalcode,"maggot.tkn");
        p3.Parse(maggot_code);
        p3.TraverseAST(p3.ast.root, 0); //print the tree

        Runner runner;
        runner.Run(p3.ast);
    }
}

int main(int argc, char* argv[])
{
    vector<string> args;
    for(int i=0; i<argc; ++i)
        args.push_back(argv[i]);

    Main(args);
}

