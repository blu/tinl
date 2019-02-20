Description
-----------

*This is Not Lisp* (**TINL**; proncounced *tai-nul*) is a minimalistic LISP-like language, using familiar s-expressions with essential restrictions:

* no lists
* no array, vector, string or function types -- just scalars
* no custom types -- only built-in types
* no *set/setq/setf* forms -- variable initializations are effectively Single Static Assignments (SSA)
* no quoted expressions
* no lambdas
* no NIL results -- all expressions must return a value
* no bignum numeric type -- only fixed-bitness integers
* no rational numeric type -- only floating-point fractions
* no binary or octal literals -- only decimal and hexidecimal (via *0x* prefix)
* no predicate forms -- *ifzero/ifneg expr then-expr else-expr* instead
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

Misc considerations
-------------------

For better cross-compatibility with Common LISP, TINL does not allow DEFUN forms among the args to a function invocation -- an artificial limitation to TINL.

```lisp
	(+ (defun foo() 42) (foo) (foo)) ; define foo in the context of this invocation of ‘+’ ‒ not allowed in LISP, and thus in TINL
```

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

To Do
-----

Read numeric input from stdin.
