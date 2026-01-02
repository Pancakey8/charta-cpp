;;; charta-mode.el --- Major mode for Charta language -*- lexical-binding: t; -*-
;;; Commentary:
;;; Basic Charta syntax support
;;; Code:
(require 'rx)

(defvar charta-mode-hook nil)

(defvar charta-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?\" "\"" st)
    (modify-syntax-entry ?\\ "\\" st)
    st))

(defconst charta-types
  '("int" "char" "bool" "string" "stack" "function" "opaque"))

(defconst charta-operators
  '("→" "←" "↑" "↓" "<-" "->" "^|" "|^" "v|" "|v" "?" "≍" "~~"))

(defun charta--match-cffi (limit)
  "Match cffi { ... } blocks with balanced braces up to LIMIT."
  (when (re-search-forward "\\<cffi\\>" limit t)
    (let ((start (match-beginning 0)))
      (save-excursion
        (goto-char (match-end 0))
        (skip-chars-forward " \t\n")
        (when (eq (char-after) ?{)
          (let* ((brace-pos (point))
                 (end (ignore-errors (scan-sexps brace-pos 1))))
            (when (and end (<= end limit))
              (set-match-data (list start end))
              t)))))))

(defun charta--extend-font-lock-region ()
  "Ensure cffi blocks are fully refontified."
  (save-excursion
    (let ((changed nil))
      ;; Extend backward to start of cffi
      (when (re-search-backward "\\<cffi\\>" nil t)
        (let ((pos (match-beginning 0)))
          (when (< pos font-lock-beg)
            (setq font-lock-beg pos
                  changed t))))
      ;; Extend forward to matching }
      (save-excursion
        (goto-char font-lock-end)
        (when (re-search-forward "\\<cffi\\>" nil t)
          (goto-char (match-end 0))
          (skip-chars-forward " \t\n")
          (when (eq (char-after) ?{)
            (let ((end (ignore-errors (scan-sexps (point) 1))))
              (when (and end (> end font-lock-end))
                (setq font-lock-end end
                      changed t))))))
      changed)))

(defconst charta-font-lock-keywords
  (list
   ;; cffi block
   '(charta--match-cffi
     (0 font-lock-string-face t))

   ;; fn keyword
   '("\\<fn\\>" . font-lock-keyword-face)

   ;; operators
   (cons (regexp-opt charta-operators)
         font-lock-builtin-face)

   ;; booleans: 'T 'F
   '("'[TF⊤⊥]\\_>\\([^']\\|\\'\\)" . font-lock-constant-face)

   '("\\(?:^\\|[[:space:]]\\)\\([+-]?[0-9]+\\(\\.[0-9]+\\)?\\)"
     1 font-lock-constant-face)

   ;; strings
   '("\"\\([^\"\\\\]\\|\\\\.\\)*\"" . font-lock-string-face)

   ;; characters
   '("'\\([^'\\\\]\\|\\\\.\\)'" . font-lock-string-face)

   ;; types
   (cons (regexp-opt charta-types 'symbols)
         font-lock-type-face)))

;;;###autoload
(define-derived-mode charta-mode prog-mode "Charta"
  "Major mode for Charta."
  :syntax-table charta-mode-syntax-table
  (setq font-lock-defaults (list charta-font-lock-keywords))
  (font-lock-mode 1)
  (font-lock-ensure)
  (add-hook 'font-lock-extend-region-functions
            #'charta--extend-font-lock-region
            nil t))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.charta\\'" . charta-mode))

(provide 'charta-mode)
;;; charta-mode.el ends here
