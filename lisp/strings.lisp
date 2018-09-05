(fn string-to-list (str)
    (let ((len (str-len str))
          (res ()))
      (do ((x 0 (+ x 1)))
          ((or
            (= x len)
            (set res (cons (str-nth str x) res)))
           res))))
             
