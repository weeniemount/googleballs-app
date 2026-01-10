const CACHE_NAME = 'googleballs-cache-v6';
const FILES = [
    '/',
    '/sw.js',
    '/index.html',
    '/manifest.json',
    '/balls.png',
    '/favicon.ico',
    '/balls.js',
    '/balls.css'
];

self.addEventListener('install', event => {
    event.waitUntil(
        caches.open(CACHE_NAME).then(cache => cache.addAll(FILES))
    );
    self.skipWaiting();
});

self.addEventListener('fetch', event => {
    event.respondWith(
        caches.match(event.request).then(res => res || fetch(event.request))
    );
});
