#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import division
import math
import operator as op

'''

The syntax of a language is the arrangement of characters to form correct statements or expressions; 
the semantics is the meaning of those statements or expressions.

'''

'''

Scheme syntax is much simpler:

Scheme programs consist solely of expressions. There is no statement/expression distinction.

Numbers (e.g. 1) and symbols (e.g. A) are called atomic expressions; they cannot be broken into pieces. These are similar to their Java counterparts, except that in Scheme, operators such as + and > are symbols too, and are treated the same way as A and fn.

Everything else is a list expression: a "(", followed by zero or more expressions, followed by a ")". 
The first element of the list determines what it means:

A list starting with a keyword, e.g. (if ...), is a special form; the meaning depends on the keyword.
A list starting with a non-keyword, e.g. (fn ...), is a function call.

'''

'''

The beauty of Scheme is that the full language only needs 5 keywords and 8 syntactic forms. 

'''

'''

A language interpreter has two parts:

Parsing: The parsing component takes an input program in the form of a sequence of characters, verifies it according to the syntactic rules of the language, and translates the program into an internal representation. In a simple interpreter the internal representation is a tree structure (often called an abstract syntax tree) that closely mirrors the nested structure of statements or expressions in the program. In a language translator called a compiler there is often a series of internal representations, starting with an abstract syntax tree, and progressing to a sequence of instructions that can be directly executed by the computer. The Lispy parser is implemented with the function parse.

Execution: The internal representation is then processed according to the semantic rules of the language, thereby carrying out the computation. Lispy's execution function is called eval (note this shadows Python's built-in function of the same name).

program ➡ [parse] ➡ abstract-syntax-tree ➡ [eval] ➡ result

'''

'''

Symbol = str              # A Scheme Symbol is implemented as a Python str
Number = (int, float)     # A Scheme Number is implemented as a Python int or float
Atom   = (Symbol, Number) # A Scheme Atom is a Symbol or Number
List   = list             # A Scheme List is implemented as a Python list
Exp    = (Atom, List)     # A Scheme expression is an Atom or List
Env    = dict             # A Scheme environment (defined below)  is a mapping of {variable: value}

'''

################ Parsing: parse, tokenize, and read_from_tokens

def parse(program):
    return read_from_tokens(tokenize(program))

def tokenize(s):
    return s.replace('(', ' ( ').replace(')', ' ) ').split()

def read_from_tokens(tokens):
    if len(tokens) == 0:
        raise SyntaxError('unexpected EOF')
    token = tokens.pop(0)
    if token == ')':
    	raise SyntaxError('unexpected )')
    elif token == '(':
    	L = []
    	while tokens[0] != ')':
    		L.append(read_from_tokens(tokens))
    	tokens.pop(0)
    	return L
    else:
    	return atom(token)

def atom(token):
    try: return int(token)
    except ValueError:
        try: return float(token)
        except ValueError:
            return str(token)

################ Environments

'''

An environment is a mapping from variable names to their values. By default, eval will use a global environment that includes the names for a bunch of standard functions (like sqrt and max, and also operators like *). This environment can be augmented with user-defined variables, using the expression (define symbol value).

'''

class Env(dict):
    "An environment: a dict of {'var':val} pairs, with an outer Env."
    def __init__(self, parms=(), args=(), outer=None):
        self.update(zip(parms, args))
        self.outer = outer
    def find(self, var):
        "Find the innermost Env where var appears."
        return self if (var in self) else self.outer.find(var)

def standard_env():
    "An environment with some Scheme standard procedures."
    env = Env()
    env.update(vars(math)) # sin, cos, sqrt, pi, ...
    env.update({
        '+':op.add, '-':op.sub, '*':op.mul, '/':op.truediv, 
        '>':op.gt, '<':op.lt, '>=':op.ge, '<=':op.le, '=':op.eq, 
        'abs':     abs,
        'append':  op.add,  
        'apply':   apply,
        'begin':   lambda *x: x[-1],
        'car':     lambda x: x[0],
        'cdr':     lambda x: x[1:], 
        'cons':    lambda x,y: [x] + y,
        'eq?':     op.is_, 
        'equal?':  op.eq, 
        'length':  len, 
        'list':    lambda *x: list(x), 
        'list?':   lambda x: isinstance(x,list), 
        'map':     map,
        'max':     max,
        'min':     min,
        'not':     op.not_,
        'null?':   lambda x: x == [], 
        'number?': lambda x: isinstance(x, Number),   
        'procedure?': callable,
        'round':   round,
        'symbol?': lambda x: isinstance(x, Symbol),
    })
    return env

global_env = standard_env()

################ Procedures

class Procedure(object):
    "A user-defined Scheme procedure."
    def __init__(self, parms, body, env):
        self.parms, self.body, self.env = parms, body, env
    def __call__(self, *args): 
        return eval(self.body, Env(self.parms, args, self.env))

################ eval

def eval(x, env=global_env):
	"Evaluate an expression in an environment."
	if isinstance(x, str):
		return env.find(x)[x]
	elif not isinstance(x, list):
		return x
	elif x[0] == 'quote':
		(_, exp) = x
		return exp
	elif x[0] == 'if':
		(_, test, conseq, alt) = x
		exp = (conseq if eval(test, env) else alt)
		return eval(exp, env)
	elif x[0] == 'define':
		(_, var, exp) = x
		env[var] = eval(exp, env)
	elif x[0] == 'set!':
		(_, var, exp) = x
		env.find(var)[var] = eval(exp, env)
	elif x[0] == 'lambda':
		(_, params, body) = x
		return Procedure(params, body, env)
	else:
		proc = eval(x[0], env)
		args = [eval(exp, env) for exp in x[1:]]
		return proc(*args)

################ Interaction: A REPL

def lispstr(exp):
    "Convert a Python object back into a Lisp-readable string."
    if isinstance(exp, list):
        return '(' + ' '.join(map(lispstr, exp)) + ')' 
    else:
        return str(exp)

def repl(prompt='lisp.py> '):
    "A prompt-read-eval-print loop."
    while True:
        val = eval(parse(raw_input(prompt)))
        if val is not None: 
            print(lispstr(val))

################ Test

program = "(begin (define r 10) (* pi (* r r)))"
print(program)
print("")
print(tokenize(program))
print("")
print(parse(program))
print("")
print(eval(parse(program)))

repl()

