package com.pocketpy.ide.data

import java.io.File

data class PocketFile(
    val name: String,
    val file: File,
    val isSample: Boolean = false
) {
    val formattedSize: String
        get() {
            val bytes = file.length()
            return when {
                bytes < 1024 -> "$bytes B"
                bytes < 1024 * 1024 -> "${bytes / 1024} KB"
                else -> String.format("%.1f MB", bytes.toDouble() / (1024 * 1024))
            }
        }
}
