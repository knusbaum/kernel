(defmacro progn (&rest body)
  `(let nil
     ,@body))

(defmacro cond (&rest args)
  (let ((cond-pair (car args))
        (rest (cdr args)))
    (let ((a (car cond-pair))
          (b (car (cdr cond-pair))))
      `(if ,a
           ,b
           ,(if rest
                `(cond ,@rest)
                nil)))))

(defmacro or (&rest args)
  (let ((sym (gensym))
        (first-elem (car args))
        (rest (cdr args)))
    `(let ((,sym ,first-elem))
       (if ,sym
           ,sym
           ,(if rest
                `(or ,@rest)
                nil)))))

(defmacro and (&rest args)
  (let ((curr-arg (car args))
        (rest (cdr args)))
    `(if ,(car args)
         ,(if (car (cdr rest))
              `(and ,@rest)
              (car rest)))))

;; The finally form will be executed even if
;; an error is thrown out of the body.
(defmacro unwind-protect (body finally)
  (let ((errsym (gensym)))
    `(progn
       (catch (nil ,errsym)
         ,body
         (progn
           ,finally
           (error ,errsym)))
       ,finally)))

(defmacro not (form)
  `(if ,form
       nil
       t))

(fn do-lets (steps bindings)
  (if steps
      (let ((currbind (car steps)))
        (do-lets (cdr steps) (append bindings (list (car currbind) (car (cdr currbind))))))
      bindings))

(fn do-increments (steps increments)
  (if steps
      (let ((currbind (car steps)))
        (do-increments (cdr steps) (append increments `(set ,(car currbind) ,(car (cdr (cdr currbind)))))))
      increments))

;;; (do ((var initform nextform)* )
;;;   (end-condition-test return-form))
(defmacro do (steps end-form)
  (let ((conditional (gensym))
        (loop-body (gensym)))
    `(let ,(do-lets steps ())
       (tagbody
          (go ,conditional)
          ,loop-body
          ,@(do-increments steps ())
          ,conditional
          (if (not ,(car end-form)) (go ,loop-body)))
       ,(car (cdr end-form)))))

(fn load-file (fname)
  (let ((f (open fname)))
    (catch ('end-of-file e)
      (do ((form (eval (read f)) (eval (read f))))
          (nil :done))
      (close f))))
