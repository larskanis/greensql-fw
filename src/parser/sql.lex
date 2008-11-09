%{
#include "sql.tab.hpp"
#include "expression.hpp"
#include "parser.hpp"
static int get_q_string(int delimeter);
int yyparse();
static YY_BUFFER_STATE buf;
%}

%%

select      return SELECT;
insert      return INSERT;
update      return UPDATE;
delete      return DELETE;
show        return SHOW;

distinctrow         return SELECT_OPT;
distinct            return DISTINCT;
all                 return SELECT_OPT;
high_priority       return SELECT_OPT;
straight_join       return STRAIGHT_JOIN;
sql_small_result    return SELECT_OPT;
sql_big_result      return SELECT_OPT;
sql_buffer_result   return SELECT_OPT;
sql_cache           return SELECT_OPT;
sql_no_cache        return SELECT_OPT;
sql_calc_found_rows return SELECT_OPT;

low_priority        return LOW_PRIORITY;
quick               return QUICK;

set                 return SET;
between             return BETWEEN;
case                return CASE;
when                return WHEN;
then                return THEN;
else                return ELSE;
end\scase           return END_CASE;
end                 return END_CASE;

index       return INDEX;
key         return INDEX;

join        return JOIN;
left        return JOIN_TYPE;
outer       return JOIN_TYPE;
right       return JOIN_TYPE;
inner       return JOIN_TYPE;
natural     return JOIN_TYPE;
cross       return JOIN_TYPE;

use         return USE;
on          return ON;
using       return USING;
ignore      return IGNORE;
force       return FORCE;
for         return FOR;

regexp      return REGEXP;

union       {clb_found_union_token(); return UNION;}
from        return FROM;
where       return WHERE;
like        return LIKE;
rlike       return LIKE;
into        return INTO;
sounds\slike return LIKE;

as          return AS;
in          return IN;

and         return AND;
&&          return AND;
or          {clb_found_or_token(); return OR;}
\|\|        {clb_found_or_token(); return OR;}
not         return NOT;

group       return GROUP;
order       return ORDER;
by          return BY;
asc         return ASC;
desc        return DESC;

is          return IS;
null        return NULLX;
\N          return NULLX;

any         return ANY;
exists      return EXISTS;

having      return HAVING;
limit       return LIMIT;
procedure   return PROCEDURE;

default     return DEFAULT;
unknwon     return UNKNOWN;
true        return TRUEX;
false       return FALSEX;

div         return DIVIDE;
xor         return XOR;

[0-9]+      {  yylval.int_val = atoi(yytext);
               return INTEGER;
	    }

version\((\ |\t|\r|\n)*\) {
              yylval.str_val = new SQLString("version()");
              return STRING;
            }

"/*"        {
              clb_found_comment();
              // using classic flex example for handling comment
              register int c;
              for ( ;1; )
              {
                while ( (c = yyinput()) != '*' && c!= EOF )
                  ;
                if (c == '*')
                {
                  while ( (c=yyinput()) == '*' )
                    ;
                  if ( c == '/' )
                    break;
                } 
                if (c == EOF)
                {
                  // fix of some strange bug that make application to hang
                  buf->yy_buffer_status = YY_BUFFER_EOF_PENDING;
                  break;
                }
              }
            }

\-\-[^\r\n]*(\r|\n)* { clb_found_comment(); }

\#[^\r\n]*(\r|\n)*   { clb_found_comment(); }

collate[ \t\r\n]+[a-z_][a-z0-9\._]* ; //ignore COLLATE language statement
with[ \t\r\n]+rollup ; // group by modifier
binary       ; // ignore binary statement

[a-z_][a-z0-9\._]* {
               yylval.str_val = new SQLString(yytext);
	       return STRING;
            }

	    
@[a-z0-9\.]+ {
               yylval.int_val = strlen(yytext);
	       return VARIABLE;
	    }

"="           return EQUAL;
"<=>"         return EQUAL;
"!="          return N_EQUAL;
"<>"          return N_EQUAL;
">="          return N_EQUAL;
"<="          return N_EQUAL;
">"           return N_EQUAL;
"<"           return N_EQUAL;
">>"          return SHIFT;
"<<"          return SHIFT;
"+"           return PLUS;
"-"           return MINUS;
"*"           return MULTIPLY;
"/"           return DIVIDE;
"^"           return POWER;
"%"           return PERCENT;
"|"           return BIT_OR;
"&"           return BIT_AND;
"~"           return BIT_NOT;
"!"           return BIT_NOT;

";"          {
               /* calculate number of bytes read so far */
               int pos = (int)(yy_c_buf_p - buf->yy_ch_buf);
               /* check that ';' is not in the end of the buffer */
               if (pos < ( buf->yy_n_chars - 1))
               {
                 clb_found_query_separator();
               }
               return END;
             }

"("          return O_BRACE;
")"          return C_BRACE;
","          return COMMA;
"."          return DOT;

"'"          return get_q_string('\'');
"\""         return get_q_string('\"');
"`"          return get_q_string('`');

[ \t\v\f\r\n]+  ; // return END;

.            { /* printf("invalid charachter %s\n", yytext); */ }

%%
//<<EOF>>      return END;


static int get_q_string(int delimeter)
{
    //printf("looking for end of string\n");
    std::string str;
    int quoted = 0;
    int c;

    while ( (c = yyinput()) != EOF)
    {
        if (c == delimeter && quoted == 0)
        {
            yylval.str_val = new SQLString(str);
            if (delimeter == '\'')
                return Q_STRING;
            else if (delimeter == '\"')
                return DQ_STRING;
            else
                return NAME;
        }
        // add new char
        str.append( 1, (unsigned char) c );
        if (c == '\\' && quoted == 0)
        {
            quoted = 1;
        } else
        {
            quoted = 0;
        }
    }
    //yyunput(EOF, buf->yy_ch_buf );
    // fix of some strange bug that make application to hang
    buf->yy_buffer_status = YY_BUFFER_EOF_PENDING;

    yylval.str_val = new SQLString(str);
    if (delimeter == '\'')
        return Q_STRING;
    else if (delimeter == '\"')
        return DQ_STRING;
    return NAME;
}

// when end of buffer is reached, stop processing
int yywrap()
{
    return 1;
}


int scan_buffer(const char * data)
{
    //printf("scanning %s\n", data);
    //YY_BUFFER_STATE buf;
    buf = yy_scan_string(data);
    yyparse();
    yy_delete_buffer(buf);
    return 1;
}
