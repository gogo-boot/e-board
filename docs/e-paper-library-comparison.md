# See Also

- [Display Layout Overview](./display-layout-overview.md)
- [Display Modes Documentation](./display-modes.md)

# E-Paper Display Libraries Comparison

This document compares the most popular libraries for drawing on e-paper displays in Arduino/C++ environments.

---

## 1. GxEPD2

- **Best for:** Most e-paper displays (Waveshare, Good Display, LilyGO, etc.)
- **Image Support:** Bitmaps (monochrome or grayscale, as C arrays or from file)
- **Pros:**
  - Optimized for e-paper refresh and partial updates
  - Supports grayscale (if display supports it)
  - Good documentation for e-paper-specific features
- **Cons:**
  - No direct SVG/vector support
  - Bitmap images must be converted to C arrays or loaded from SD card

---

## 2. Adafruit GFX (with Adafruit EPD)

- **Best for:** General graphics on many displays (including some e-paper via Adafruit EPD)
- **Image Support:** Bitmaps (monochrome or color, as C arrays)
- **Pros:**
  - Simple API, lots of drawing primitives
  - Works with many display types
- **Cons:**
  - Not optimized for e-paper refresh cycles
  - No direct SVG/vector support

---

## 3. U8g2

- **Best for:** Drawing text and simple graphics on monochrome and some grayscale displays
- **Image Support:** XBM (monochrome bitmap) images, as C arrays
- **Pros:**
  - Excellent font and text rendering
  - Many built-in fonts and symbols
- **Cons:**
  - Limited to monochrome or 4-level grayscale
  - No direct support for color or vector images

---

## Summary Table

| Library      | Best for         | Image Support         | Vector/SVG | Grayscale | Notes                        |
|--------------|------------------|----------------------|------------|-----------|------------------------------|
| GxEPD2       | E-paper displays | Bitmap (C array/file)| No         | Yes*      | *If display supports it      |
| Adafruit GFX | Many displays    | Bitmap (C array)     | No         | No/Yes*   | *Some color/gray displays    |
| U8g2         | Mono/gray text   | XBM (C array)        | No         | Yes*      | *4-level gray on some HW     |

---

## Best Practice for Printing Images

1. **Convert your image (SVG, PNG, JPG, etc.) to a bitmap** (monochrome or grayscale) that matches your display’s capabilities.
   - Use tools like [Image2cpp](https://javl.github.io/image2cpp/), GIMP, or Inkscape for conversion.
   - Export as a C array or raw bitmap file.

2. **Use the display library’s bitmap drawing function**:
   - For **GxEPD2**: `display.drawBitmap(x, y, bitmap, w, h, color);`
   - For **Adafruit GFX**: `display.drawBitmap(x, y, bitmap, w, h, color);`
   - For **U8g2**: `u8g2.drawXBMP(x, y, w, h, bitmap);`

3. **If you need to display large or complex images**, consider loading from SD card or SPIFFS (if your library and hardware support it).

---

## Notes

- **Direct SVG/vector rendering is not supported** on microcontroller e-paper libraries. Always convert to bitmap first.
- Choose the library that best matches your hardware and your needs (text, graphics, refresh speed, etc.).
