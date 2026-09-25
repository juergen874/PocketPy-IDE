package com.pocketpy.ide.data

import android.content.Context
import java.io.File

class FileManager(private val context: Context) {

    val workspaceDir: File
        get() = File(context.filesDir, "workspace").also { if (!it.exists()) it.mkdirs() }

    fun initializeWorkspace() {
        val files = listFiles()
        if (files.isEmpty()) {
            restoreSamples()
        }
    }

    fun listFiles(): List<PocketFile> {
        val dir = workspaceDir
        if (!dir.exists()) return emptyList()
        return dir.listFiles()
            ?.filter { it.isFile && (it.name.endsWith(".py") || it.name.endsWith(".html") || it.name.endsWith(".json") || it.name.endsWith(".txt")) }
            ?.sortedBy { it.name }
            ?.map { PocketFile(it.name, it) }
            ?: emptyList()
    }

    fun readFile(file: PocketFile): String {
        return if (file.file.exists()) file.file.readText() else ""
    }

    fun saveFile(file: PocketFile, content: String) {
        file.file.writeText(content)
    }

    fun createFile(name: String, content: String = ""): PocketFile {
        val actualName = if (!name.contains(".")) "$name.py" else name
        val file = File(workspaceDir, actualName)
        file.writeText(content)
        return PocketFile(actualName, file)
    }

    fun renameFile(pocketFile: PocketFile, newName: String): PocketFile {
        val target = File(workspaceDir, newName)
        pocketFile.file.renameTo(target)
        return PocketFile(newName, target)
    }

    fun deleteFile(pocketFile: PocketFile): Boolean {
        return pocketFile.file.delete()
    }

    fun restoreSamples() {
        for ((name, content) in SampleScripts.SAMPLES) {
            val file = File(workspaceDir, name)
            file.writeText(content)
        }
    }
}
