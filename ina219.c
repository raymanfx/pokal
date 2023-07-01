#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "ina219.h"

struct ina219 g_ina219 = {0};

uint16_t ina219_reg_read(uint8_t reg) {
    uint8_t tmpi[2];

    i2c_write_blocking(i2c1, INA219_ADDRESS, &reg, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c1, INA219_ADDRESS, tmpi, 2, false);
    return (((uint16_t)tmpi[0] << 8) | (uint16_t)tmpi[1]);
}

void ina219_reg_write(uint8_t reg, uint16_t value) {
    uint8_t tmpi[3];

    tmpi[0] = reg;
    tmpi[1] = (value >> 8) & 0xFF;
    tmpi[2] = value & 0xFF;

    i2c_write_blocking(i2c1, INA219_ADDRESS, tmpi, 3, false); // true to keep master control of bus
}

// Setups the HW (defaults to 32V and 2A for calibration values)
void ina219_init(struct ina219 *dev) {
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(6, GPIO_FUNC_I2C);
    gpio_set_function(7, GPIO_FUNC_I2C);
    gpio_pull_up(6);
    gpio_pull_up(7);

    // By default we use a pretty huge range for the input voltage,
    // which probably isn't the most appropriate choice for system
    // that don't use a lot of power.  But all of the calculations
    // are shown below if you want to change the settings.  You will
    // also need to change any relevant register settings, such as
    // setting the VBUS_MAX to 16V instead of 32V, etc.

    // VBUS_MAX = 32V             (Assumes 32V, can also be set to 16V)
    // VSHUNT_MAX = 0.32          (Assumes Gain 8, 320mV, can also be 0.16, 0.08, 0.04)
    // RSHUNT = 0.1               (Resistor value in ohms)

    // 1. Determine max possible current
    // MaxPossible_I = VSHUNT_MAX / RSHUNT
    // MaxPossible_I = 3.2A

    // 2. Determine max expected current
    // MaxExpected_I = 2.0A

    // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
    // MinimumLSB = MaxExpected_I/32767
    // MinimumLSB = 0.000061              (61uA per bit)
    // MaximumLSB = MaxExpected_I/4096
    // MaximumLSB = 0,000488              (488uA per bit)

    // 4. Choose an LSB between the min and max values
    //    (Preferrably a roundish number close to MinLSB)
    // CurrentLSB = 0.0001 (100uA per bit)

    // 5. Compute the calibration register
    // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
    // Cal = 4096 (0x1000)

    dev->cal = 4096;

    // 6. Calculate the power LSB
    // PowerLSB = 20 * CurrentLSB
    // PowerLSB = 0.002 (2mW per bit)

    // 7. Compute the maximum current and shunt voltage values before overflow
    //
    // Max_Current = Current_LSB * 32767
    // Max_Current = 3.2767A before overflow
    //
    // If Max_Current > Max_Possible_I then
    //    Max_Current_Before_Overflow = MaxPossible_I
    // Else
    //    Max_Current_Before_Overflow = Max_Current
    // End If
    //
    // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
    // Max_ShuntVoltage = 0.32V
    //
    // If Max_ShuntVoltage >= VSHUNT_MAX
    //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
    // Else
    //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
    // End If

    // 8. Compute the Maximum Power
    // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
    // MaximumPower = 3.2 * 32V
    // MaximumPower = 102.4W

    // Set multipliers to convert raw current/power values
    dev->current_div_mA = 1.0; // Current LSB = 100uA per bit (1000/100 = 10)
    dev->power_div_mW = 20; // Power LSB = 1mW per bit (2/1)

    // Set Calibration register to 'Cal' calculated above
    ina219_reg_write(INA219_REG_CALIBRATION, dev->cal);

    // Set Config register to take into account the settings above
    uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_32S_17MS |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
    ina219_reg_write(INA219_REG_CONFIG, config);
}

static void ina219_ensure_init() {
    static bool g_init = false;

    if (g_init) {
        return;
    }

    ina219_init(&g_ina219);
    g_init = true;
}

float ina219_voltage(struct ina219 *dev) {
    uint16_t value;

    ina219_ensure_init();

    value = ina219_reg_read(INA219_REG_BUSVOLTAGE);
    // Shift to the right 3 to drop CNVR and OVF and multiply by LSB
    return (int16_t)((value >> 3) * 4) * 0.001;
}

float ina219_current(struct ina219 *dev) {
    uint16_t value;

    ina219_ensure_init();

    // Sometimes a sharp load will reset the INA219, which will
    // reset the cal register, meaning CURRENT and POWER will
    // not be available ... avoid this by always setting a cal
    // value even if it's an unfortunate extra step
    ina219_reg_write(INA219_REG_CALIBRATION, dev->cal);

    // Now we can safely read the CURRENT register!
    value = ina219_reg_read(INA219_REG_CURRENT);
    return (int16_t)value / dev->current_div_mA;
}

float ina219_percentage(struct ina219 *dev) {
    // maximum voltage for Li-ion batteries
    float v_max = 4.2;
    // minimum voltage for Li-ion batteries
    float v_min = 3.0;

    ina219_ensure_init();

    return (ina219_voltage(&g_ina219) - v_min) / (v_max - v_min) * 100;
}
