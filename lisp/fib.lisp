(fn fib (n)
    (if (< n 3)
        1
        (+ (fib (- n 1)) (fib (- n 2)))))

(fn fast-fib (n)
    (do ((temp 0 prev)
         (prev 1 curr)
         (curr 0 (+ curr temp))
         (c 0 (+ c 1)))
        ((= c n) curr)))
