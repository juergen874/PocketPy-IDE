package com.pocketpy.ide.ui.editor

import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import com.pocketpy.ide.ui.theme.*

object PythonSyntaxHighlighter {

    private val KEYWORDS = setOf(
        "def", "class", "if", "elif", "else", "while", "for", "in",
        "return", "yield", "break", "continue", "pass", "raise",
        "try", "except", "finally", "import", "from", "as",
        "lambda", "with", "assert", "del", "global", "nonlocal",
        "True", "False", "None", "and", "or", "not", "is"
    )

    private val BUILTINS = setOf(
        "print", "len", "range", "str", "int", "float", "bool",
        "list", "dict", "set", "tuple", "type", "open", "sum",
        "min", "max", "abs", "round", "enumerate", "zip", "map", "filter"
    )

    private val TOKEN_REGEX = Regex(
        "(#[^\\n]*)" +                      // 1: Comments
        "|(\"\"\"[\\s\\S]*?\"\"\"|'''[\\s\\S]*?''')" + // 2: Triple strings
        "|(\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"|'[^'\\\\]*(?:\\\\.[^'\\\\]*)*')" + // 3: Single strings
        "|\\b(0x[0-9a-fA-F]+|\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)\\b" + // 4: Numbers
        "|\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b" + // 5: Identifiers
        "|([+\\-*/%&|^=<>!]=?|[:;,.\\[\\](){}])" // 6: Operators & Delimiters
    )

    fun highlight(text: String): AnnotatedString {
        return buildAnnotatedString {
            append(text)
            for (match in TOKEN_REGEX.findAll(text)) {
                val range = match.range
                when {
                    match.groups[1] != null -> {
                        addStyle(SpanStyle(color = SyntaxComment), range.first, range.last + 1)
                    }
                    match.groups[2] != null || match.groups[3] != null -> {
                        addStyle(SpanStyle(color = SyntaxString), range.first, range.last + 1)
                    }
                    match.groups[4] != null -> {
                        addStyle(SpanStyle(color = SyntaxNumber), range.first, range.last + 1)
                    }
                    match.groups[5] != null -> {
                        val word = match.groups[5]!!.value
                        when {
                            word in KEYWORDS -> addStyle(SpanStyle(color = SyntaxKeyword, fontWeight = FontWeight.Bold), range.first, range.last + 1)
                            word in BUILTINS -> addStyle(SpanStyle(color = SyntaxBuiltin), range.first, range.last + 1)
                        }
                    }
                    match.groups[6] != null -> {
                        addStyle(SpanStyle(color = SyntaxOperator), range.first, range.last + 1)
                    }
                }
            }
        }
    }
}
