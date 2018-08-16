;; Sample naive fibonacci sequence implementation.
(fn fib (x)
    (cond ((< x 3) 1)
          (t (+ (fib (- x 1))
                (fib (- x 2))))))

(fib 10)
