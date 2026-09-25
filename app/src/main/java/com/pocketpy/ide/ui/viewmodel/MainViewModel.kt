package com.pocketpy.ide.ui.viewmodel

import android.app.Application
import androidx.compose.ui.text.input.TextFieldValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.pocketpy.ide.data.FileManager
import com.pocketpy.ide.data.PocketFile
import com.pocketpy.ide.engine.PocketPyCallback
import com.pocketpy.ide.engine.PocketPyEngine
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class MainViewModel(application: Application) : AndroidViewModel(application) {

    private val fileManager = FileManager(application)
    private val pocketPyEngine = PocketPyEngine.getInstance(application)

    private val _files = MutableStateFlow<List<PocketFile>>(emptyList())
    val files: StateFlow<List<PocketFile>> = _files.asStateFlow()

    private val _activeFile = MutableStateFlow<PocketFile?>(null)
    val activeFile: StateFlow<PocketFile?> = _activeFile.asStateFlow()

    private val _codeState = MutableStateFlow(TextFieldValue(""))
    val codeState: StateFlow<TextFieldValue> = _codeState.asStateFlow()

    private val _terminalOutput = MutableStateFlow("")
    val terminalOutput: StateFlow<String> = _terminalOutput.asStateFlow()

    private val _htmlOutput = MutableStateFlow("")
    val htmlOutput: StateFlow<String> = _htmlOutput.asStateFlow()

    private val _isRunning = MutableStateFlow(false)
    val isRunning: StateFlow<Boolean> = _isRunning.asStateFlow()

    private val _selectedTab = MutableStateFlow(0)
    val selectedTab: StateFlow<Int> = _selectedTab.asStateFlow()

    private var executionJob: Job? = null

    init {
        fileManager.initializeWorkspace()
        refreshFiles()
        val defaultFile = _files.value.firstOrNull()
        if (defaultFile != null) {
            selectFile(defaultFile)
        }
    }

    fun refreshFiles() {
        _files.value = fileManager.listFiles()
    }

    fun selectFile(file: PocketFile) {
        saveCurrentFile()
        _activeFile.value = file
        val content = fileManager.readFile(file)
        _codeState.value = TextFieldValue(content)

        if (file.name.endsWith(".html")) {
            _htmlOutput.value = content
        }
    }

    fun onCodeChange(newValue: TextFieldValue) {
        _codeState.value = newValue
    }

    fun saveCurrentFile() {
        val file = _activeFile.value ?: return
        fileManager.saveFile(file, _codeState.value.text)
    }

    fun createFile(name: String) {
        val file = fileManager.createFile(name)
        refreshFiles()
        selectFile(file)
    }

    fun renameFile(file: PocketFile, newName: String) {
        val updated = fileManager.renameFile(file, newName)
        refreshFiles()
        if (_activeFile.value?.file?.path == file.file.path) {
            _activeFile.value = updated
        }
    }

    fun deleteFile(file: PocketFile) {
        fileManager.deleteFile(file)
        refreshFiles()
        if (_activeFile.value?.file?.path == file.file.path) {
            val next = _files.value.firstOrNull()
            if (next != null) {
                selectFile(next)
            } else {
                _activeFile.value = null
                _codeState.value = TextFieldValue("")
            }
        }
    }

    fun restoreSamples() {
        fileManager.restoreSamples()
        refreshFiles()
        _files.value.firstOrNull()?.let { selectFile(it) }
    }

    fun selectTab(index: Int) {
        _selectedTab.value = index
    }

    fun clearTerminal() {
        _terminalOutput.value = ""
    }

    fun clearVisual() {
        _htmlOutput.value = ""
    }

    fun runScript() {
        if (_isRunning.value) return
        saveCurrentFile()

        val code = _codeState.value.text
        val filename = _activeFile.value?.name ?: "script.py"

        _selectedTab.value = 1 // Switch to terminal
        _isRunning.value = true
        _terminalOutput.value = "=== Running with PocketPy: $filename ===\n\n"

        val callback = object : PocketPyCallback {
            override fun onOutput(text: String) {
                _terminalOutput.value += text
            }

            override fun onError(text: String) {
                _terminalOutput.value += "\u001B[31m$text\u001B[0m"
            }
        }

        executionJob = viewModelScope.launch {
            val startTime = System.currentTimeMillis()
            val result = pocketPyEngine.executeScript(code, filename, callback)
            val elapsed = System.currentTimeMillis() - startTime

            _terminalOutput.value += if (result.success) {
                "\n\u001B[32m=== Process finished in ${elapsed}ms ===\u001B[0m\n"
            } else {
                "\n\u001B[31m=== Process failed with error (${elapsed}ms) ===\u001B[0m\n"
            }

            // Extract HTML for Visual Tab if present
            if (code.contains("<!DOCTYPE html") || code.contains("<html")) {
                val start = code.indexOf("<html")
                val end = code.lastIndexOf("</html>")
                if (start != -1 && end != -1) {
                    _htmlOutput.value = code.substring(start, end + 7)
                }
            }

            _isRunning.value = false
        }
    }

    fun stopScript() {
        executionJob?.cancel()
        _isRunning.value = false
        _terminalOutput.value += "\n\u001B[33m=== Process stopped by user ===\u001B[0m\n"
    }
}
