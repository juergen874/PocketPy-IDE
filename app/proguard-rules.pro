# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Keep PocketPy engine classes, callbacks and data models
-keep class com.pocketpy.ide.engine.** { *; }
-keepclassmembers class com.pocketpy.ide.engine.** { *; }
-keep interface com.pocketpy.ide.engine.PocketPyCallback { *; }
-keep class * implements com.pocketpy.ide.engine.PocketPyCallback { *; }
-keepclassmembers class * implements com.pocketpy.ide.engine.PocketPyCallback { *; }

# Keep ViewModel callbacks and properties
-keep class com.pocketpy.ide.ui.viewmodel.** { *; }
-keepclassmembers class com.pocketpy.ide.ui.viewmodel.** { *; }

# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep WebView and JavaScript interfaces
-keepattributes JavascriptInterface
-keepclassmembers class * {
    @android.webkit.JavascriptInterface <methods>;
}
-keep class android.webkit.** { *; }
-keepclassmembers class * extends android.webkit.WebViewClient { *; }
