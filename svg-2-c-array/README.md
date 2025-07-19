## SVG to C Header Conversion Guide

This guide explains how to convert SVG files into C header files containing bitmap arrays for use with the AdafruitGFX library.

---

### Overview

The script `svg_to_headers.sh` will:
- Convert SVG files in the `./svg` directory to PNGs of the specified size
- Convert those PNGs to C header files, each containing a bitmap array
- Output the header files in a new directory: `./icons`

**To use the new icons in your PlatformIO project:**
Manually move the generated `icons` folder to `/lib/bitmap_images`.

---

### Usage

```sh
bash svg_to_headers.sh <size_of_output_image>
```

---

### Dependencies

- **Inkscape**: Used via CLI to convert SVG to PNG
- **Python3**: To run `png_to_header.py` (in this folder)
- **Pillow**: Python image library (install in venv, see below)

---

### Regenerating All Icons

1. **Create a virtual environment:**
   ```sh
   python3 -m venv venv
   ```
2. **Activate the virtual environment:**
   ```sh
   source venv/bin/activate
   ```
3. **Install Pillow inside the venv:**
   ```sh
   pip install pillow
   ```
4. **Run the make command:**
   ```sh
   make
   ```
5. **Move generated Icons**
   ```sh
   mv icons/* ../lib/bitmap_images
   ```

**Note:**
- Do NOT run `make -j`. Inkscape has a known bug when running SVG to PNG conversions in parallel. See: https://gitlab.com/inkscape/inkscape/-/issues/4716

---

### License Notice

The icons in the `svg` sub-directory remain licensed under their original license agreements. See citations below for more details.

---

### Citations

**Weather Icons (`wi-*.svg`)**
- Source: https://github.com/erikflowers/weather-icons
- Weather Icons: SIL OFL 1.1 (http://scripts.sil.org/OFL)
- Code: MIT License (http://opensource.org/licenses/mit-license.html)
- Documentation: CC BY 3.0 (http://creativecommons.org/licenses/by/3.0)

**Battery Icons (`battery*.svg`) & Visibility Icon (`visibility_icon.svg`)**
- Source: https://fonts.google.com/icons
- License: Apache License 2.0 (https://www.apache.org/licenses/LICENSE-2.0.txt)

**House Icon (`house.svg`)**
- Source: https://seekicon.com/free-icon/house_16
- License: MIT License (http://opensource.org/licenses/mit-license.html)

**WiFi Icons (`wifi*.svg`), Warning Alert (`warning_icon.svg`, `error_icon.svg`)**
- Source: https://github.com/phosphor-icons/homepage
- License: MIT License (http://opensource.org/licenses/mit-license.html)

**House Icon (`house.svg`) and Derived Icons**
- Source: https://seekicon.com/free-icon/house_16
- License: MIT License (http://opensource.org/licenses/mit-license.html)
- Derived icons (`house_temperature.svg`, `house_humidity.svg`, `house_rainsdrops.svg`) created by transforming icons from https://github.com/erikflowers/weather-icons with `house.svg` and are licensed under SIL OFL 1.1 (http://scripts.sil.org/OFL)

**Ionizing Radiation Symbol (`ionizing_radiation_symbol.svg`)**
- Source: https://svgsilh.com/image/309911.html
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)

**Biological Hazard Symbol (`biological_hazard_symbol.svg`)**
- Source: https://svgsilh.com/image/37775.html
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)

**Wind Direction Icons (`meteorological_wind_direction_**deg.svg`)**
- Source: https://www.onlinewebfonts.com/icon/251550
- License: CC BY 3.0 (http://creativecommons.org/licenses/by/3.0)
