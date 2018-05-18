//cc.js

// token比较大的分类
var TOKEN_STYLE = [
    'KEY_WORD', 'IDENTIFIER', 'DIGIT_CONSTANT',
    'OPERATOR', 'SEPARATOR', 'STRING_CONSTANT'
];

// 将关键字、运算符、分隔符进行具体化
var DETAIL_TOKEN_STYLE = {
    'include': 'INCLUDE',
    'int': 'INT',
    'float': 'FLOAT',
    'char': 'CHAR',
    'double': 'DOUBLE',
    'for': 'FOR',
    'if': 'IF',
    'else': 'ELSE',
    'while': 'WHILE',
    'do': 'DO',
    'return': 'RETURN',
    '=': 'ASSIGN',
    '&': 'ADDRESS',
    '<': 'LT',
    '>': 'GT',
    '++': 'SELF_PLUS',
    '--': 'SELF_MINUS',
    '+': 'PLUS',
    '-': 'MINUS',
    '*': 'MUL',
    '/': 'DIV',
    '>=': 'GET',
    '<=': 'LET',
    '(': 'LL_BRACKET',
    ')': 'RL_BRACKET',
    '{': 'LB_BRACKET',
    '}': 'RB_BRACKET',
    '[': 'LM_BRACKET',
    ']': 'RM_BRACKET',
    ',': 'COMMA',
    '"': 'DOUBLE_QUOTE',
    ';': 'SEMICOLON',
    '#': 'SHARP',
};

// 关键字
var keywords = [
    ['int', 'float', 'double', 'char', 'void'],
    ['if', 'for', 'while', 'do', 'else'], ['include', 'return'],
];

// 运算符
var operators = [
    '=', '&', '<', '>', '++', '--', '+', '-', '*', '/', '>=', '<=', '!='
];

// 分隔符
var delimiters = ['(', ')', '{', '}', '[', ']', ',', '\"', ';'];

// c文件名字
var file_name = undefined;

// 文件内容
var content = undefined;
//content = "int main(){ int a = 2, b = 3; if (a < b) { return a; } else { return b; } }";

function isalpha(c) 
{
	return (c>='a'&&c<='z') || (c>='A'&&c<='Z');
}

function isdigit(c) 
{
	return (c>='0'&&c<='9');
}

// Token 类 : 记录分析出来的单词
function Token(type_index, value)
{
	if (type_index == 0 || type_index == 3 || type_index == 4) {
		this.type = DETAIL_TOKEN_STYLE[value];
	} else {
		this.type = TOKEN_STYLE[type_index];
	}
	
	this.value = value;
}

// Lexer 类 : 词法分析器
function Lexer()
{
	this.tokens = [];
}

Lexer.prototype.is_blank = function(index) {
	var c = content[index];
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
};

Lexer.prototype.skip_blank = function(index) {
	while (index < content.length && this.is_blank(index)) {
		index += 1;
	}
	return index;
};

Lexer.prototype.is_keyword = function(value) {
	for (var item in keywords) {
		if (keywords[item].indexOf(value) > -1) {
			return true;
		}
	}
	return false;
};

// Lexer主分析程序
Lexer.prototype.main = function() {
	var i = 0;
	while (i < content.length) {
		i = this.skip_blank(i);
		if (content[i] == '#') {
			this.tokens.push(new Token(4, content[i]));
			i = this.skip_blank(i+1);
			while (i < content.length) {
				if (content.slice(i).search('include')) {
					this.tokens.push(new Token(0, 'include'));
					i = this.skip_blank(i+7);
				}
				else if (content[i] == '\"' || content[i] == '<') {
					this.tokens.push(new Token(4, content[i]));
					i = this.skip_blank(i+1);
					var close_flag = '>';
					if (content[i] == '\"') {
						close_flag = '\"'
					}
					var lib = '';
					while (content[i] != close_flag) {
						lib += content[i];
						i += 1;
					}
					this.tokens.push(new Token(1, lib));
					this.tokens.push(new Token(4, close_flag));
					i = this.skip_blank(i+1);
				} else {
					// 'include error!'
					console.log('include error!');
					//exit();
				}
			}
		}
		else if(isalpha(content[i]) || content[i] == '_') {
			var temp = '';
			while (i < content.length && (isalpha(content[i]) || content[i] == '_' || isdigit(content[i]))) {
				temp += content[i];
				i += 1;
			}
			if (this.is_keyword(temp)) {
				this.tokens.push(new Token(0, temp));
			} else {
				this.tokens.push(new Token(1, temp));
			}
			i = this.skip_blank(i);
		}
		else if (isdigit(content[i])) {
			var temp = '';
			while (i < content.length) {
				if (isdigit(content[i]) || (content[i] == '.' && isdigit(content[i+1]))) {
					temp += content[i];
					i += 1;
				} else if (!isdigit(content[i])) {
					if (content[i] == '.') {
						console.log('float number error!');
						//exit();
					} else {
						break;
					}
				}
			}
			this.tokens.push(new Token(2, temp));
			i = this.skip_blank(i);
		}
		else if (delimiters.indexOf(content[i]) > -1) {
			this.tokens.push(new Token(4, content[i]));
			if (content[i] == '\"') {
				i += 1;
				var temp = '';
				while (i < content.length) {
					if (content[i] != '\"') {
						temp += content[i];
						i += 1;
					} else {
						break;
					}
				}
				if (i >= content.length) {
					console.log('error:lack of \"');
					//exit();
				}
				this.tokens.push(new Token(5, temp));
				this.tokens.push(new Token(4, '\"'));
			}
			i = this.skip_blank(i+1);
		}
		else if (operators.indexOf(content[i]) > -1) {
			if ((content[i] == '+' || content[i] == '-') && (content[i] == content[i+1])) {
				this.tokens.push(new Token(3, content[i]+content[i]));
				i = this.skip_blank(i+2);
			} else if ((content[i] == '>' || content[i] == '<') && content[i + 1] == '=') {
				this.tokens.push(new Token(3, content[i]+'='));
				i = this.skip_blank(i+2);
			} else {
				this.tokens.push(new Token(3, content[i]));
				i = this.skip_blank(i+1);
			}
		}
	}
};

Lexer.prototype.show = function() {
	this.main();
	for (var i in this.tokens) {
		console.log("(%s, %s)", this.tokens[i].type, this.tokens[i].value);
	}
};




