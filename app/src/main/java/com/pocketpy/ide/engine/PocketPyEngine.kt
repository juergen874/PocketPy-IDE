package com.pocketpy.ide.engine

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioManager
import android.media.ToneGenerator
import android.os.BatteryManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.speech.tts.TextToSpeech
import android.widget.Toast
import androidx.core.app.NotificationCompat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.util.Locale

data class ExecutionResult(
    val success: Boolean,
    val error: String
)

interface PocketPyCallback {
    fun onOutput(text: String)
    fun onError(text: String)
}

class PocketPyEngine private constructor(private val context: Context) : TextToSpeech.OnInitListener {

    private val mainHandler = Handler(Looper.getMainLooper())
    private var tts: TextToSpeech? = null
    private var isTtsReady = false

    init {
        try {
            System.loadLibrary("pocketpy_engine")
        } catch (e: UnsatisfiedLinkError) {
            e.printStackTrace()
        }
        try {
            tts = TextToSpeech(context, this)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun onInit(status: Int) {
        if (status == TextToSpeech.SUCCESS) {
            try {
                tts?.language = Locale.GERMAN
            } catch (e: Exception) {
                tts?.language = Locale.getDefault()
            }
            isTtsReady = true
        }
    }

    // --- Android Bridge Methods called from JNI ---

    fun showToast(msg: String, isLong: Boolean) {
        mainHandler.post {
            try {
                Toast.makeText(context, msg, if (isLong) Toast.LENGTH_LONG else Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    fun vibratePhone(ms: Long) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                val vibratorManager = context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as? VibratorManager
                vibratorManager?.defaultVibrator?.vibrate(VibrationEffect.createOneShot(ms, VibrationEffect.DEFAULT_AMPLITUDE))
            } else {
                @Suppress("DEPRECATION")
                val vibrator = context.getSystemService(Context.VIBRATOR_SERVICE) as? Vibrator
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    vibrator?.vibrate(VibrationEffect.createOneShot(ms, VibrationEffect.DEFAULT_AMPLITUDE))
                } else {
                    @Suppress("DEPRECATION")
                    vibrator?.vibrate(ms)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun showNotification(title: String, text: String, id: Int) {
        try {
            val channelId = "pocketpy_notifications"
            val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as? NotificationManager ?: return

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                val channel = NotificationChannel(channelId, "PocketPy IDE", NotificationManager.IMPORTANCE_DEFAULT).apply {
                    description = "PocketPy Script Notifications"
                }
                notificationManager.createNotificationChannel(channel)
            }

            val notification = NotificationCompat.Builder(context, channelId)
                .setSmallIcon(android.R.drawable.ic_dialog_info)
                .setContentTitle(title)
                .setContentText(text)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setAutoCancel(true)
                .build()

            notificationManager.notify(id, notification)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun speakText(text: String) {
        mainHandler.post {
            try {
                if (isTtsReady && tts != null) {
                    tts?.speak(text, TextToSpeech.QUEUE_FLUSH, null, "pocketpy_tts")
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    fun getBatteryLevel(): Int {
        return try {
            val batteryStatus: Intent? = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
            val level: Int = batteryStatus?.getIntExtra(BatteryManager.EXTRA_LEVEL, -1) ?: -1
            val scale: Int = batteryStatus?.getIntExtra(BatteryManager.EXTRA_SCALE, -1) ?: -1
            if (level >= 0 && scale > 0) ((level / scale.toFloat()) * 100).toInt() else 100
        } catch (e: Exception) {
            100
        }
    }

    fun isBatteryCharging(): Boolean {
        return try {
            val batteryStatus: Intent? = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
            val status: Int = batteryStatus?.getIntExtra(BatteryManager.EXTRA_STATUS, -1) ?: -1
            status == BatteryManager.BATTERY_STATUS_CHARGING || status == BatteryManager.BATTERY_STATUS_FULL
        } catch (e: Exception) {
            false
        }
    }

    fun copyToClipboard(text: String) {
        mainHandler.post {
            try {
                val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
                val clip = ClipData.newPlainText("PocketPy", text)
                clipboard?.setPrimaryClip(clip)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    fun getClipboard(): String {
        return try {
            val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
            val item = clipboard?.primaryClip?.getItemAt(0)
            item?.text?.toString() ?: ""
        } catch (e: Exception) {
            ""
        }
    }

    fun beep(freq: Int, durationMs: Int) {
        try {
            val toneGen = ToneGenerator(AudioManager.STREAM_NOTIFICATION, 100)
            toneGen.startTone(ToneGenerator.TONE_PROP_BEEP, durationMs)
        } catch (e: Exception) {
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
