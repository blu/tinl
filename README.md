Description
-----------

*This is Not Lisp* (**TINL**; proncounced *tai-nul*) is a minimalistic LISP-like language, with the following limitations:

* no lists
* no array, vector, string or function types
* no custom types -- only built-in types
* no *set/setq/setf* forms -- TINL is purely functional
* no quoted expressions
* no lambdas
* no NIL results -- all expressions must return a value
* no bignum numeric type -- only fixed-bitness integers
* no rational numeric type -- only floating-point fractions
* no binary or octal literals -- only decimal and hexidecimal (via *0x* prefix)
* no predicate forms -- *ifzero/ifneg expr then-expr else-expr* instead
* no *dotimes* et al loop forms -- loops only via recursion

Example of correspondence between LISP and TINL
-----------------------------------------------

Print `n + 2` consecutive members of the Fibonacci sequence. Examples intentionally written for maximum likeness.

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
