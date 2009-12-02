%{
#include "sql.tab.hpp"
#include "expression.hpp"
#include "parser.hpp"
static int get_q_string(int delimeter);
int yyparse();
static YY_BUFFER_STATE buf;
%}

%option never-interactive

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

cast                return CAST;
unsigned            return DATA_TYPE;
signed              return DATA_TYPE;
time                return DATA_TYPE;
decimal             return DATA_TYPE;
datetime            return DATA_TYPE;
binary              return BINARY;
char                return CHAR_TYPE;
varbinary           return DATA_TYPE;
varchar             return DATA_TYPE;
integer             return DATA_TYPE;
bigint              return DATA_TYPE;
int                 return DATA_TYPE;
float               return DATA_TYPE;
double[ \t\v\f\r\n\xA0]+precision return DATA_TYPE; // mysql
double              return DATA_TYPE;
real                return DATA_TYPE;
dec                 return DATA_TYPE; //mysql dec - decimal
salary              return DATA_TYPE; //mysql
blob                return DATA_TYPE;
text                return DATA_TYPE;
tiny(blob|text|int) return DATA_TYPE; //mysql specific
small(blob|text|int) return DATA_TYPE; //mysql specific
medium(blob|text|int) return DATA_TYPE; //mysql specific
long(blob|text)     return DATA_TYPE; //mysql specific
timestamp           return DATA_TYPE;
long[ \t\v\f\r\n\xA0]+varchar return DATA_TYPE;
longvar(char|binary) return DATA_TYPE;
long                return DATA_TYPE;
bit                 return DATA_TYPE;

zerofill            ; // mysql specific

latin1_(bin|german2_ci) ; //return ENCODING;
_?latin1                ; //return ENCODING;
utf8_bin                ; //return ENCODING;
utf8                    ; //return ENCODING;
_bin                    ; //return ENCODING;

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
sounds[ \t\v\f\r\n\xA0]+like return LIKE;

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

b[ \t\v\f\r\n\xA0]*'[0-9]+'   { yylval.int_val = 3; return INTEGER; }
0x[0-9a-f]+ {  yylval.int_val = 3; return INTEGER; }

[0-9]+      {  yylval.int_val = atoi(yytext);
               return INTEGER;
	    }

version\([ \t\v\f\r\n\xA0]*\) {
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

collate[ \t\v\f\r\n\xA0]+[a-z_][a-z0-9\._]* ; //ignore COLLATE language statement
with[ \t\v\f\r\n\xA0]+rollup ; // group by modifier

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

[\%\+\-\~\!]+[ \t\v\f\r\n\xA0\+\-\~\!\%]*           return BASIC_OP;

"*"           return MULTIPLY;
"/"           return DIVIDE;
"^"           return POWER;
"|"           return BIT_OR;
"&"           return BIT_AND;

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

[ \t\v\f\r\n\xA0]+  ; // return END;

.            { /* printf("invalid charachter %s\n", yytext); */ }

%%
//<<EOF>>      return END;


static int get_q_string(int delimeter)
{
    //printf("looking for end of string\n");
    std::string str;
    int quoted = 0;
    int c;
    int n;

    while ( (c = yyinput()) != EOF)
    {
        if (c == delimeter && quoted == 0)
        {
            // handle the following queries:
            // SELECT 'start''end'
            if (delimeter == '\'' || delimeter == '"')
            {
              n = yyinput();
              if (n == delimeter)
              {
                // continue as is
                str.append( 1, (unsigned char) c );
                continue; 
              } else if (n == EOF)
              {
                buf->yy_buffer_status = YY_BUFFER_EOF_PENDING;

                yylval.str_val = new SQLString(str);
                if (delimeter == '\'')
                  return Q_STRING;
                return DQ_STRING;
              }
              // else return back next char
              yyunput(n, buf->yy_ch_buf );
            }
            
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
