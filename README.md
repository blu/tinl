Description
-----------

*This is Not Lisp* (**TINL**, proncounced *tai-nul*) is a minimalistic LISP-like language, using familiar s-expressions with essential restrictions:

* no lists
* no array, vector, string or function types -- just scalars
* no *defstruct* structured types either
* no *set/setq/setf* forms -- variable initializations are effectively Single Static Assignments (SSA)
* no quoted expressions
* no lambdas
* no NIL results -- all expressions must return a value
* no bignum numeric type -- only fixed-bitness integers
* no rational numeric type -- only floating-point fractions
* no binary or octal literals -- only decimal and hexidecimal (via *0x* prefix)
* no T/NIL predicate forms -- *ifzero/ifneg expr then-expr else-expr* instead
* no *dotimes* et al loop forms -- loops only via recursion

Built-in functions
------------------

`+ arg1 arg2 {..}` -- compute arg1 + arg2 + .. + argN; promote `int` args to `float` if at least one arg is `float`  
`- arg1 arg2 {..}` -- compute arg1 - arg2 - .. - argN; promote `int` args to `float` if at least one arg is `float`  
`* arg1 arg2 {..}` -- compute arg1 * arg2 * .. * argN; promote `int` args to `float` if at least one arg is `float`  
`/ arg1 arg2 {..}` -- compute arg1 / arg2 / .. / argN; promote `int` args to `float` if at least one arg is `float`  
`ifzero arg1 arg2 arg3` -- if arg1 is zero, compute arg2, otherwise compute arg3  
`ifneg arg1 arg2 arg3` -- if arg1 is negative, compute arg2, otherwise compute arg3  
`print arg` -- print arg and return arg; exert side-effect  
`readi32`, `readf32` -- read a scalar of the respective type from stdin

Example of correspondence between LISP and TINL
-----------------------------------------------

Print `n + 2` consecutive members of the Fibonacci sequence. Maximum code likeness sought.

Common LISP:

```lisp
	(defun fib(x y n)
		(print x) (if (zerop n) (print y) (fib y (+ x y) (- n 1))))
```

TINL:

```lisp
	(defun fib(x y n)
		(print x) (ifzero n (print y) (fib y (+ x y) (- n 1))))
```

Example of partial-evaluation (PE) optimisations in TINL
--------------------------------------------------------

Print first 5 members of the Fibonacci sequence, return the 5th as result:

```sh
$ echo "(defun fib(x y n) (print x) (ifzero n (print y) (fib y (+ x y) (- n 1)))) (fib 1 1 3)" | ./tinl # ./tinl fib_short_sid.tinl
ASTNODE_LET: unknown fib
  ASTNODE_INIT: unknown x (2)
  ASTNODE_INIT: unknown y (3)
  ASTNODE_INIT: unknown n (4)
  ASTNODE_EVAL_FUN: unknown print
    ASTNODE_EVAL_VAR: unknown x (2)
  ASTNODE_EVAL_FUN: unknown ifzero
    ASTNODE_EVAL_VAR: unknown n (4)
    ASTNODE_EVAL_FUN: unknown print
      ASTNODE_EVAL_VAR: unknown y (3)
    ASTNODE_EVAL_FUN: unknown fib
      ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown +
        ASTNODE_EVAL_VAR: unknown x (2)
        ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown -
        ASTNODE_EVAL_VAR: unknown n (4)
        ASTNODE_LITERAL: i32 1
ASTNODE_EVAL_FUN: unknown fib
  ASTNODE_LITERAL: i32 1
  ASTNODE_LITERAL: i32 1
  ASTNODE_LITERAL: i32 3
success
1
1
2
3
5
i32 5
ASTNODE_LET: unknown fib
  ASTNODE_INIT: unknown x (2)
  ASTNODE_INIT: unknown y (3)
  ASTNODE_INIT: unknown n (4)
  ASTNODE_EVAL_FUN: unknown print
    ASTNODE_EVAL_VAR: unknown x (2)
  ASTNODE_EVAL_FUN: unknown ifzero
    ASTNODE_EVAL_VAR: unknown n (4)
    ASTNODE_EVAL_FUN: unknown print
      ASTNODE_EVAL_VAR: unknown y (3)
    ASTNODE_EVAL_FUN: unknown fib
      ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown +
        ASTNODE_EVAL_VAR: unknown x (2)
        ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown -
        ASTNODE_EVAL_VAR: unknown n (4)
        ASTNODE_LITERAL: i32 1
ASTNODE_LET: i32
  ASTNODE_INIT: i32 x (2)
    ASTNODE_LITERAL: i32 1
  ASTNODE_INIT: i32 y (3)
    ASTNODE_LITERAL: i32 1
  ASTNODE_INIT: i32 n (4)
    ASTNODE_LITERAL: i32 3
  ASTNODE_EVAL_FUN: i32 print
    ASTNODE_LITERAL: i32 1
  ASTNODE_LET: i32
    ASTNODE_INIT: i32 x (2)
      ASTNODE_LITERAL: i32 1
    ASTNODE_INIT: i32 y (3)
      ASTNODE_LITERAL: i32 2
    ASTNODE_INIT: i32 n (4)
      ASTNODE_LITERAL: i32 2
    ASTNODE_EVAL_FUN: i32 print
      ASTNODE_LITERAL: i32 1
    ASTNODE_LET: i32
      ASTNODE_INIT: i32 x (2)
        ASTNODE_LITERAL: i32 2
      ASTNODE_INIT: i32 y (3)
        ASTNODE_LITERAL: i32 3
      ASTNODE_INIT: i32 n (4)
        ASTNODE_LITERAL: i32 1
      ASTNODE_EVAL_FUN: i32 print
        ASTNODE_LITERAL: i32 2
      ASTNODE_LET: i32
        ASTNODE_INIT: i32 x (2)
          ASTNODE_LITERAL: i32 3
        ASTNODE_INIT: i32 y (3)
          ASTNODE_LITERAL: i32 5
        ASTNODE_INIT: i32 n (4)
          ASTNODE_LITERAL: i32 0
        ASTNODE_EVAL_FUN: i32 print
          ASTNODE_LITERAL: i32 3
        ASTNODE_EVAL_FUN: i32 print
          ASTNODE_LITERAL: i32 5
```

Return the 5th member of the Fibonacci sequence:

```sh
$ echo "(defun fib(x y n) (ifzero n y (fib y (+ x y) (- n 1)))) (fib 1 1 3)" | ./tinl # ./tinl fib_short_lit.tinl
ASTNODE_LET: unknown fib
  ASTNODE_INIT: unknown x (2)
  ASTNODE_INIT: unknown y (3)
  ASTNODE_INIT: unknown n (4)
  ASTNODE_EVAL_FUN: unknown ifzero
    ASTNODE_EVAL_VAR: unknown n (4)
    ASTNODE_EVAL_VAR: unknown y (3)
    ASTNODE_EVAL_FUN: unknown fib
      ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown +
        ASTNODE_EVAL_VAR: unknown x (2)
        ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown -
        ASTNODE_EVAL_VAR: unknown n (4)
        ASTNODE_LITERAL: i32 1
ASTNODE_EVAL_FUN: unknown fib
  ASTNODE_LITERAL: i32 1
  ASTNODE_LITERAL: i32 1
  ASTNODE_LITERAL: i32 3
success
i32 5
ASTNODE_LET: unknown fib
  ASTNODE_INIT: unknown x (2)
  ASTNODE_INIT: unknown y (3)
  ASTNODE_INIT: unknown n (4)
  ASTNODE_EVAL_FUN: unknown ifzero
    ASTNODE_EVAL_VAR: unknown n (4)
    ASTNODE_EVAL_VAR: unknown y (3)
    ASTNODE_EVAL_FUN: unknown fib
      ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown +
        ASTNODE_EVAL_VAR: unknown x (2)
        ASTNODE_EVAL_VAR: unknown y (3)
      ASTNODE_EVAL_FUN: unknown -
        ASTNODE_EVAL_VAR: unknown n (4)
        ASTNODE_LITERAL: i32 1
ASTNODE_LITERAL: i32 5
```
