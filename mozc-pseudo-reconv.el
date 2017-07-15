(require 'decompjp)
(require 'boiled-mozc)

(when (and (fboundp 'module-load)
           (locate-library "reverse-translate-driver-mecab-module"))
  (require 'reverse-translate-driver-mecab-module)

  (defvar mecab-rcfile nil
    "If string, use it as MeCab resource file for dynamic module version
reverse-translate-driver-mecab.
If changed after first call of dcj:reverse-translate-driver-mecab-module,
must call dcj:reverse-translate-driver-mecab-module-initialize.")

  (advice-add 'dcj:reverse-translate-driver-mecab
              :override 'dcj:reverse-translate-driver-mecab-module))

(defvar mozc-pseudo-reconv-search-begin nil
  "If the value is a function, it is called to search the reconversion start position.")

(defvar mozc-pseudo-reconv-original
  "Original string before reconversion.")
(make-variable-buffer-local 'mozc-pseudo-reconv-original)
(put 'mozc-pseudo-reconv-original 'permanent-local t)

(defvar im-before-mozc-pseudo-reconv
  "Input method before reconversion.")
(make-variable-buffer-local 'im-before-mozc-pseudo-reconv)
(put 'im-before-mozc-pseudo-reconv 'permanent-local t)

(defun before-mozc-handle-event-advice (&rest arg)
  (setq boiled-mozc-conv-original mozc-pseudo-reconv-original))

(defun after-boiled-mozc-deactivate-advice ()
  (if im-before-mozc-pseudo-reconv
      (activate-input-method im-before-mozc-pseudo-reconv))
  (advice-remove 'boiled-mozc-deactivate-input-method
                 #'after-boiled-mozc-deactivate-advice))

(defun mozc-pseudo-reconv ()
 "Pseudo reconversion for mozc.el."
  (interactive)
  (setq im-before-mozc-pseudo-reconv current-input-method)
  (when (or (not buffer-read-only) inhibit-read-only)
    (let ((pt (point)) spt start end kana)
      (if (region-active-p)
          (setq start (min (mark) pt) end (max (mark) pt))
        (setq spt (funcall
                   (or (and (functionp mozc-pseudo-reconv-search-begin)
                            mozc-pseudo-reconv-search-begin)
                       'dcj:reverse-search-begin))
              start (min spt pt) end (max spt pt)))
      (setq mozc-pseudo-reconv-original (buffer-substring start end)
            kana (dcj:reverse-kanji-to-kana mozc-pseudo-reconv-original))
      (when kana
        (delete-region start end)
        (goto-char start)
        (insert (dcj:reverse-kana-to-romaji kana))
        (advice-add 'boiled-mozc-deactivate-input-method :after
                    #'after-boiled-mozc-deactivate-advice)
        (advice-add 'mozc-handle-event :before
                    #'before-mozc-handle-event-advice)
        (boiled-mozc-rK-conv)
        (advice-remove 'mozc-handle-event
                       #'before-mozc-handle-event-advice)))))

(provide 'mozc-pseudo-reconv)
