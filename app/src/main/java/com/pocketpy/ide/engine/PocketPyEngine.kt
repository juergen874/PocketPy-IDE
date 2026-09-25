package com.pocketpy.ide.engine

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

data class ExecutionResult(
    val success: Boolean,
    val error: String
)

interface PocketPyCallback {
    fun onOutput(text: String)
    fun onError(text: String)
}

class PocketPyEngine private constructor(private val context: Context) {

    init {
        try {
            System.loadLibrary("pocketpy_engine")
        } catch (e: UnsatisfiedLinkError) {
            e.printStackTrace()
        }
    }

    private external fun nativeInit(): Boolean
    private external fun nativeExecute(code: String, filename: String, callback: PocketPyCallback?): ExecutionResult

    suspend fun executeScript(
        code: String,
        filename: String = "<script>",
        callback: PocketPyCallback? = null
    ): ExecutionResult = withContext(Dispatchers.IO) {
        try {
            nativeExecute(code, filename, callback)
        } catch (t: Throwable) {
            callback?.onError("Execution Error: ${t.localizedMessage}\n")
            ExecutionResult(false, t.localizedMessage ?: "Unknown Error")
        }
    }

    companion object {
        @Volatile
        private var instance: PocketPyEngine? = null

        fun getInstance(context: Context): PocketPyEngine {
            return instance ?: synchronized(this) {
                instance ?: PocketPyEngine(context.applicationContext).also { instance = it }
            }
        }
    }
}
