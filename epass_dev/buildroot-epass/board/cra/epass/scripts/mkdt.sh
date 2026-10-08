#!/bin/bash
set -o pipefail

echo "Generating devicetree and dt overlays..."

DIR_IN="board/cra/epass/devicetree/linux"
BASE_DIR_IN="${DIR_IN}/base"
SCREEN_DIR_IN="${DIR_IN}/screen"
INTERFACE_DIR_IN="${DIR_IN}/interface"
EXT_DIR_IN="${DIR_IN}/ext"

DIR_OUT="${BINARIES_DIR}/dt"
BASE_DIR_OUT="${DIR_OUT}/base"
SCREEN_DIR_OUT="${DIR_OUT}/screen"
INTERFACE_DIR_OUT="${DIR_OUT}/interface"
EXT_DIR_OUT="${DIR_OUT}/ext"

TEMP_DIR="${DIR_OUT}/temp"

rm -r "${DIR_OUT}"

mkdir -p "${BASE_DIR_OUT}"
mkdir -p "${SCREEN_DIR_OUT}"
mkdir -p "${INTERFACE_DIR_OUT}"
mkdir -p "${EXT_DIR_OUT}"
mkdir -p "${TEMP_DIR}"

echo "Generating base devicetree..."
for basedts in "${BASE_DIR_IN}"/*.dts; do
    echo "Generating ${basedts}..."
    cpp -nostdinc -I "${BUILD_DIR}/linux-5.4.99/include/" \
        -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
        -P -undef -x assembler-with-cpp -o "${TEMP_DIR}/$(basename ${basedts}).dts" \
        "${basedts}"
    dtc -@ -I dts -O dtb -o "${BASE_DIR_OUT}/$(basename ${basedts} .dts).dtb" "${TEMP_DIR}/$(basename ${basedts}).dts"
    rm "${TEMP_DIR}/$(basename ${basedts}).dts"
done

echo "Generating interface dtoverlays..."
for interfacedts in "${INTERFACE_DIR_IN}"/*.dts; do
    echo "Generating ${interfacedts}..."
    cpp -nostdinc -I "${BUILD_DIR}/linux-5.4.99/include/" \
        -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
        -P -undef -x assembler-with-cpp -o "${TEMP_DIR}/$(basename ${interfacedts}).dts" \
        "${interfacedts}"
    dtc -@ -I dts -O dtb -o "${INTERFACE_DIR_OUT}/$(basename ${interfacedts} .dts).dtbo" "${TEMP_DIR}/$(basename ${interfacedts}).dts"
    rm "${TEMP_DIR}/$(basename ${interfacedts}).dts"
done

echo "Generating ext dtoverlay..."
for extdts in "${EXT_DIR_IN}"/*.dts; do
    echo "Generating ${extdts}..."
    cpp -nostdinc -I "${BUILD_DIR}/linux-5.4.99/include/" \
        -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
        -P -undef -x assembler-with-cpp -o "${TEMP_DIR}/$(basename ${extdts}).dts" \
        "${extdts}"
    dtc -@ -I dts -O dtb -o "${EXT_DIR_OUT}/$(basename ${extdts} .dts).dtbo" "${TEMP_DIR}/$(basename ${extdts}).dts"
    rm "${TEMP_DIR}/$(basename ${extdts}).dts"
done

echo "Generating screen dtoverlay..."
for screendts in "${SCREEN_DIR_IN}"/*.dts; do
    echo "Generating ${screendts}..."
    cpp -nostdinc -I "${BUILD_DIR}/linux-5.4.99/include/" \
        -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
        -P -undef -x assembler-with-cpp -o "${TEMP_DIR}/$(basename ${screendts}).dts" \
        "${screendts}"
    dtc -@ -I dts -O dtb -o "${SCREEN_DIR_OUT}/$(basename ${screendts} .dts).dtbo" "${TEMP_DIR}/$(basename ${screendts}).dts"
    rm "${TEMP_DIR}/$(basename ${screendts}).dts"
done

rm -r "${TEMP_DIR}"
