#if !defined(ESP32_TRNG)
#define ESP32_TRNG

#if defined(ESP32)

// https://rweather.github.io/arduinolibs/RNG_8cpp_source.html

int trng() {
    return esp_random();
}

#endif

#endif
