package com.weenie.googleballs

import android.os.Build
import android.webkit.WebView
import androidx.compose.runtime.Composable
import androidx.compose.ui.viewinterop.AndroidView
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.ui.Modifier
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            LocalWebView()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                window.statusBarColor = Color.Black.toArgb()
            }
        }
    }
}

@Composable
fun LocalWebView() {
    AndroidView(
        factory = { context ->
            WebView(context).apply {
                settings.javaScriptEnabled = true
                settings.domStorageEnabled = true // important for older WebViews
                settings.useWideViewPort = true
                settings.loadWithOverviewMode = true
                loadUrl("file:///android_asset/balls.html")
            }
        },
        modifier = Modifier // Simplified modifier usage
            .fillMaxSize() // Make sure WebView fills the screen
    )
}
