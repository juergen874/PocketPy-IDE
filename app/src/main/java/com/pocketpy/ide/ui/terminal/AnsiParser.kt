package com.pocketpy.ide.ui.terminal

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import com.pocketpy.ide.ui.theme.*

object AnsiParser {

    private val ANSI_REGEX = Regex("\u001B\\[([0-9;]*)m")

    fun parse(text: String): AnnotatedString {
        return buildAnnotatedString {
            var lastIndex = 0
            var currentColor = IdeTextPrimary

            for (match in ANSI_REGEX.findAll(text)) {
                if (match.range.first > lastIndex) {
                    val segment = text.substring(lastIndex, match.range.first)
                    pushStyle(SpanStyle(color = currentColor))
                    append(segment)
                    pop()
                }

                val codes = match.groupValues[1].split(';').mapNotNull { it.toIntOrNull() }
                for (code in codes) {
                    when (code) {
                        0 -> currentColor = IdeTextPrimary
                        31, 91 -> currentColor = IdeError
                        32, 92 -> currentColor = IdeSuccess
                        33, 93 -> currentColor = IdeWarning
                        34, 94 -> currentColor = IdePrimary
                        35, 95 -> currentColor = IdeSecondary
                        36, 96 -> currentColor = IdeTertiary
                    }
                }
                lastIndex = match.range.last + 1
            }

            if (lastIndex < text.length) {
                pushStyle(SpanStyle(color = currentColor))
                append(text.substring(lastIndex))
                pop()
            }
        }
    }
}
