package com.pocketpy.ide.ui.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.OffsetMapping
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.text.input.TransformedText
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pocketpy.ide.ui.theme.*

class PythonVisualTransformation : VisualTransformation {
    override fun filter(text: androidx.compose.ui.text.AnnotatedString): TransformedText {
        return TransformedText(
            PythonSyntaxHighlighter.highlight(text.text),
            OffsetMapping.Identity
        )
    }
}

@Composable
fun CodeEditorView(
    textFieldValue: TextFieldValue,
    onValueChange: (TextFieldValue) -> Unit,
    modifier: Modifier = Modifier,
    fontSizeSp: Int = 14
) {
    val scrollState = rememberScrollState()
    val horizontalScrollState = rememberScrollState()
    val visualTransformation = remember { PythonVisualTransformation() }

    val lines = remember(textFieldValue.text) {
        val count = textFieldValue.text.count { it == '\n' } + 1
        (1..count).toList()
    }

    val quickSymbols = remember {
        listOf(
            "Tab" to "    ",
            ":" to ":",
            "=" to " = ",
            "(" to "(",
            ")" to ")",
            "[" to "[",
            "]" to "]",
            "{" to "{",
            "}" to "}",
            "\"" to "\"",
            "'" to "'",
            "def" to "def ",
            "import" to "import ",
            "print" to "print(",
            "if" to "if ",
            "else" to "else:\n    ",
            "for" to "for i in range():",
            "while" to "while ",
            "try" to "try:\n    \nexcept Exception as e:\n    print(e)",
            "self." to "self.",
            "lambda" to "lambda ",
            "#" to "# "
        )
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(IdeSurface)
    ) {
        // Quick Symbols Bar
        Surface(
            color = IdeSurfaceVariant,
            modifier = Modifier.fillMaxWidth()
        ) {
            LazyRow(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 8.dp, vertical = 6.dp),
                horizontalArrangement = Arrangement.spacedBy(6.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                items(quickSymbols) { (label, insertText) ->
                    FilledTonalButton(
                        onClick = {
                            val current = textFieldValue.text
                            val sel = textFieldValue.selection
                            val newText = current.substring(0, sel.start) + insertText + current.substring(sel.end)
                            val newCursor = sel.start + insertText.length
                            onValueChange(
                                textFieldValue.copy(
                                    text = newText,
                                    selection = TextRange(newCursor)
                                )
                            )
                        },
                        colors = ButtonDefaults.filledTonalButtonColors(
                            containerColor = IdeSurface,
                            contentColor = IdeTextPrimary
                        ),
                        shape = RoundedCornerShape(6.dp),
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 2.dp),
                        modifier = Modifier.height(28.dp)
                    ) {
                        Text(
                            text = label,
                            fontSize = 11.sp,
                            fontFamily = FontFamily.Monospace
                        )
                    }
                }
            }
        }

        // Code Editor Body with Line Numbers
        Row(
            modifier = Modifier
                .fillMaxSize()
                .weight(1f)
                .verticalScroll(scrollState)
        ) {
            // Line numbers column
            Column(
                modifier = Modifier
                    .width(42.dp)
                    .background(IdeSurfaceElevated)
                    .padding(vertical = 12.dp, horizontal = 4.dp),
                horizontalAlignment = Alignment.End
            ) {
                lines.forEach { lineNum ->
                    Text(
                        text = "$lineNum",
                        color = IdeTextMuted,
                        fontSize = fontSizeSp.sp,
                        fontFamily = FontFamily.Monospace,
                        lineHeight = (fontSizeSp + 6).sp
                    )
                }
            }

            // Editable Text Area
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(start = 12.dp, top = 12.dp, end = 12.dp, bottom = 48.dp)
                    .horizontalScroll(horizontalScrollState)
            ) {
                BasicTextField(
                    value = textFieldValue,
                    onValueChange = onValueChange,
                    textStyle = TextStyle(
                        color = IdeTextPrimary,
                        fontSize = fontSizeSp.sp,
                        fontFamily = FontFamily.Monospace,
                        lineHeight = (fontSizeSp + 6).sp
                    ),
                    cursorBrush = SolidColor(IdePrimary),
                    visualTransformation = visualTransformation,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }
    }
}
