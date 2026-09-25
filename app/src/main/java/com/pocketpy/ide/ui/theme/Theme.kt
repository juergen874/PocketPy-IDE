package com.pocketpy.ide.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable

private val DarkColorScheme = darkColorScheme(
    primary = IdePrimary,
    secondary = IdeSecondary,
    tertiary = IdeTertiary,
    background = IdeBackground,
    surface = IdeSurface,
    onPrimary = IdeBackground,
    onSecondary = IdeBackground,
    onTertiary = IdeBackground,
    onBackground = IdeTextPrimary,
    onSurface = IdeTextPrimary
)

@Composable
fun PocketPyTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = DarkColorScheme,
        typography = Typography,
        content = content
    )
}
