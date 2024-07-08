#pragma once

#include <string>
#include <sstream>

using namespace std;
inline vector<string> breakStringForDisplaying(string str, int charPerLine) {

        vector<string> result(1);
        std::istringstream stm(str) ; // use a string stream to split on white-space
        std::string word ;
        while( stm >> word ) // for each word in the string
        {
                // if this word will fit into the current line, append the word to the current line
                if( ( result.back().size() + word.size() ) <= charPerLine ) {
                        result.back() += word + ' ' ;
                }
                else
        {
                        result.back().pop_back() ; // remove the trailing space at the end of the current line
                        result.push_back( word + ' ' ) ; // and place this new word on the next line
                }
        }

        result.back().pop_back() ; // remove the trailing space at the end of the last line
        return result ;

}
