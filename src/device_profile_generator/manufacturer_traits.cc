// Copyright 2024 Normal Browser Authors. All rights reserved.
// manufacturer_traits.cc — Full 11-OEM manufacturer traits database.

#include "device_profile_generator/manufacturer_traits.h"

#include <cmath>
#include <random>

namespace normal_browser {

namespace {

// Base Android fonts present on ALL manufacturers
const std::vector<std::string> kBaseAndroidFonts = {
    "Roboto",
    "NotoSansCJK",
    "NotoSansDevanagari",
    "NotoSansBengali",
    "NotoSansTamil",
    "NotoSansTelugu",
    "NotoSansKannada",
    "NotoSansMalayalam",
    "DroidSansFallback",
    "Courier New",
    "Georgia",
    "Verdana",
    "Times New Roman",
};

std::vector<ManufacturerTraits> BuildDatabase() {
  std::vector<ManufacturerTraits> db;

  // ============================
  // 1. SAMSUNG (28%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kSamsung;
    t.name = "Samsung";
    t.market_weight = 0.28f;

    t.compatible_socs = {
        {"snapdragon_8_gen3", 0.08f, PriceTier::kPremium},
        {"snapdragon_8_gen2", 0.10f, PriceTier::kFlagship},
        {"snapdragon_8_gen1", 0.08f, PriceTier::kFlagship},
        {"exynos_2400", 0.10f, PriceTier::kFlagship},
        {"exynos_2200", 0.07f, PriceTier::kFlagship},
        {"snapdragon_7_plus_gen2", 0.10f, PriceTier::kMidRange},
        {"exynos_1380", 0.15f, PriceTier::kMidRange},
        {"snapdragon_6_gen1", 0.10f, PriceTier::kMidRange},
        {"snapdragon_695", 0.08f, PriceTier::kBudget},
        {"helio_g99", 0.07f, PriceTier::kBudget},
        {"helio_g85", 0.04f, PriceTier::kBudget},
        {"helio_g36", 0.03f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1612, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f, 2.75f}};
    t.flagship_screen = {1080, 1440, 2340, 3200, {2.75f, 3.0f, 3.5f}};

    t.budget_ram = {3072, 4096};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288, 16384};

    t.camera_configs = {{1, 2}, {1, 3}, {1, 4}, {2, 4}};

    t.custom_fonts = {
        "SamsungOne", "SamsungSans", "SECRobotoLight", "SECRoboto",
        "SamsungOneUI", "GothicA1", "SamsungColorEmoji",
        "Comic Sans MS"};

    t.system_ui_font = "SamsungOne";
    t.emoji_font = "SamsungColorEmoji";

    t.sensor_sets = {
        {"LSM6DSO Accelerometer", "STMicroelectronics",
         "LSM6DSO Gyroscope", "STMicroelectronics",
         "AK09918 Magnetometer", "AKM"},
        {"LSM6DSL Accelerometer", "STMicroelectronics",
         "LSM6DSL Gyroscope", "STMicroelectronics",
         "AK09918 Magnetometer", "AKM"},
        {"ICM-42607 Accelerometer", "InvenSense",
         "ICM-42607 Gyroscope", "InvenSense",
         "MMC5603 Magnetometer", "MEMSIC"},
    };

    t.build_fp_format =
        "samsung/%cxx%r1/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%B";

    db.push_back(std::move(t));
  }

  // ============================
  // 2. XIAOMI (22%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kXiaomi;
    t.name = "Xiaomi";
    t.market_weight = 0.22f;

    t.compatible_socs = {
        {"snapdragon_8_gen3", 0.06f, PriceTier::kPremium},
        {"snapdragon_8_gen2", 0.08f, PriceTier::kFlagship},
        {"dimensity_9300", 0.05f, PriceTier::kFlagship},
        {"dimensity_9200", 0.05f, PriceTier::kFlagship},
        {"snapdragon_7_plus_gen2", 0.08f, PriceTier::kMidRange},
        {"snapdragon_7_gen1", 0.10f, PriceTier::kMidRange},
        {"dimensity_8300", 0.07f, PriceTier::kMidRange},
        {"dimensity_8200", 0.06f, PriceTier::kMidRange},
        {"dimensity_7200", 0.07f, PriceTier::kMidRange},
        {"snapdragon_695", 0.08f, PriceTier::kBudget},
        {"helio_g99", 0.10f, PriceTier::kBudget},
        {"helio_g85", 0.08f, PriceTier::kBudget},
        {"dimensity_6080", 0.06f, PriceTier::kBudget},
        {"helio_g36", 0.06f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1650, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f, 2.75f}};
    t.flagship_screen = {1080, 1440, 2400, 3200, {2.75f, 3.0f, 3.5f}};

    t.budget_ram = {3072, 4096, 6144};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288, 16384};

    t.camera_configs = {{1, 2}, {1, 3}, {1, 4}};

    t.custom_fonts = {
        "MiSans", "MiSansVF", "MiSansLatin", "MiLanPro",
        "NotoColorEmoji"};

    t.system_ui_font = "MiSans";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "AK09918 Magnetometer", "AKM"},
        {"ICM-42607 Accelerometer", "InvenSense",
         "ICM-42607 Gyroscope", "InvenSense",
         "MMC5603 Magnetometer", "MEMSIC"},
        {"LSM6DSO Accelerometer", "STMicroelectronics",
         "LSM6DSO Gyroscope", "STMicroelectronics",
         "AK09918 Magnetometer", "AKM"},
    };

    t.build_fp_format =
        "Xiaomi/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 3. REALME (10%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kRealme;
    t.name = "Realme";
    t.market_weight = 0.10f;

    t.compatible_socs = {
        {"snapdragon_8_gen2", 0.05f, PriceTier::kFlagship},
        {"dimensity_9200", 0.05f, PriceTier::kFlagship},
        {"dimensity_8300", 0.10f, PriceTier::kMidRange},
        {"dimensity_7200", 0.12f, PriceTier::kMidRange},
        {"dimensity_7050", 0.10f, PriceTier::kMidRange},
        {"snapdragon_695", 0.12f, PriceTier::kBudget},
        {"dimensity_6100_plus", 0.12f, PriceTier::kBudget},
        {"dimensity_6080", 0.10f, PriceTier::kBudget},
        {"helio_g99", 0.10f, PriceTier::kBudget},
        {"helio_g85", 0.07f, PriceTier::kBudget},
        {"helio_g36", 0.07f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1612, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f}};
    t.flagship_screen = {1080, 1080, 2400, 2412, {2.75f, 3.0f}};

    t.budget_ram = {3072, 4096};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288, 16384};

    t.camera_configs = {{1, 2}, {1, 3}, {1, 4}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"MC3419 Accelerometer", "mCube",
         "", "",  // No gyro on many Realme budget
         "MMC5603 Magnetometer", "MEMSIC"},
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "AK09918 Magnetometer", "AKM"},
    };

    t.build_fp_format =
        "realme/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 4. ONEPLUS (8%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kOnePlus;
    t.name = "OnePlus";
    t.market_weight = 0.08f;

    t.compatible_socs = {
        {"snapdragon_8_gen3", 0.12f, PriceTier::kPremium},
        {"snapdragon_8_gen2", 0.15f, PriceTier::kFlagship},
        {"snapdragon_8_gen1", 0.10f, PriceTier::kFlagship},
        {"dimensity_9200", 0.08f, PriceTier::kFlagship},
        {"snapdragon_7_plus_gen2", 0.12f, PriceTier::kMidRange},
        {"dimensity_8300", 0.10f, PriceTier::kMidRange},
        {"dimensity_7050", 0.10f, PriceTier::kMidRange},
        {"snapdragon_695", 0.10f, PriceTier::kBudget},
        {"dimensity_6080", 0.08f, PriceTier::kBudget},
        {"helio_g85", 0.05f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 1080, 1600, 2400, {2.0f, 2.625f}};
    t.mid_screen = {1080, 1080, 2400, 2412, {2.625f, 2.75f}};
    t.flagship_screen = {1080, 1440, 2400, 3216, {2.75f, 3.0f, 3.5f}};

    t.budget_ram = {4096, 6144};
    t.mid_ram = {8192, 12288};
    t.flagship_ram = {12288, 16384};
    t.premium_ram = {16384};

    t.camera_configs = {{1, 2}, {1, 3}, {2, 4}};

    t.custom_fonts = {
        "OnePlusSans", "OnePlusSansMono", "OnePlusSansDisplay",
        "SlateForOnePlus", "NotoColorEmoji"};

    t.system_ui_font = "OnePlusSans";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "AK09918 Magnetometer", "AKM"},
        {"LSM6DSO Accelerometer", "STMicroelectronics",
         "LSM6DSO Gyroscope", "STMicroelectronics",
         "AK09918 Magnetometer", "AKM"},
    };

    t.build_fp_format =
        "OnePlus/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 5. VIVO (8%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kVivo;
    t.name = "Vivo";
    t.market_weight = 0.08f;

    t.compatible_socs = {
        {"snapdragon_8_gen3", 0.06f, PriceTier::kPremium},
        {"snapdragon_8_gen2", 0.08f, PriceTier::kFlagship},
        {"dimensity_9300", 0.06f, PriceTier::kFlagship},
        {"dimensity_8300", 0.10f, PriceTier::kMidRange},
        {"snapdragon_7_gen1", 0.10f, PriceTier::kMidRange},
        {"dimensity_7200", 0.10f, PriceTier::kMidRange},
        {"dimensity_7050", 0.10f, PriceTier::kMidRange},
        {"snapdragon_695", 0.08f, PriceTier::kBudget},
        {"helio_g99", 0.10f, PriceTier::kBudget},
        {"dimensity_6080", 0.08f, PriceTier::kBudget},
        {"helio_g85", 0.07f, PriceTier::kBudget},
        {"helio_g36", 0.07f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1612, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f}};
    t.flagship_screen = {1080, 1440, 2400, 3200, {2.75f, 3.0f}};

    t.budget_ram = {3072, 4096};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288, 16384};

    t.camera_configs = {{1, 2}, {1, 3}, {1, 4}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "MMC5603 Magnetometer", "MEMSIC"},
        {"LSM6DSO Accelerometer", "STMicroelectronics",
         "LSM6DSO Gyroscope", "STMicroelectronics",
         "AK09918 Magnetometer", "AKM"},
    };

    t.build_fp_format = "vivo/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 6. OPPO (6%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kOPPO;
    t.name = "OPPO";
    t.market_weight = 0.06f;

    t.compatible_socs = {
        {"snapdragon_8_gen2", 0.08f, PriceTier::kFlagship},
        {"dimensity_9200", 0.06f, PriceTier::kFlagship},
        {"dimensity_8300", 0.12f, PriceTier::kMidRange},
        {"dimensity_7200", 0.10f, PriceTier::kMidRange},
        {"dimensity_7050", 0.12f, PriceTier::kMidRange},
        {"snapdragon_695", 0.10f, PriceTier::kBudget},
        {"dimensity_6100_plus", 0.10f, PriceTier::kBudget},
        {"helio_g99", 0.10f, PriceTier::kBudget},
        {"helio_g85", 0.10f, PriceTier::kBudget},
        {"helio_g36", 0.12f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1612, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f}};
    t.flagship_screen = {1080, 1080, 2400, 2412, {2.75f, 3.0f}};

    t.budget_ram = {3072, 4096};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288, 16384};

    t.camera_configs = {{1, 2}, {1, 3}, {1, 4}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "MMC5603 Magnetometer", "MEMSIC"},
    };

    t.build_fp_format = "OPPO/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 7. MOTOROLA (5%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kMotorola;
    t.name = "Motorola";
    t.market_weight = 0.05f;

    t.compatible_socs = {
        {"snapdragon_8_gen2", 0.06f, PriceTier::kFlagship},
        {"snapdragon_8_gen1", 0.05f, PriceTier::kFlagship},
        {"snapdragon_7_gen1", 0.10f, PriceTier::kMidRange},
        {"dimensity_7050", 0.12f, PriceTier::kMidRange},
        {"snapdragon_695", 0.12f, PriceTier::kBudget},
        {"dimensity_6080", 0.10f, PriceTier::kBudget},
        {"helio_g99", 0.12f, PriceTier::kBudget},
        {"helio_g85", 0.15f, PriceTier::kBudget},
        {"helio_g36", 0.10f, PriceTier::kBudget},
        {"snapdragon_4_gen2", 0.08f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1612, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f}};
    t.flagship_screen = {1080, 1080, 2400, 2400, {2.75f}};

    t.budget_ram = {3072, 4096};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288};

    t.camera_configs = {{1, 2}, {1, 3}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "MMC5603 Magnetometer", "MEMSIC"},
        {"MC3419 Accelerometer", "mCube",
         "", "",  // Budget Moto often no gyro
         "MMC5603 Magnetometer", "MEMSIC"},
    };

    t.build_fp_format =
        "motorola/%c/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 8. IQOO (4%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kIQOO;
    t.name = "iQOO";
    t.market_weight = 0.04f;

    t.compatible_socs = {
        {"snapdragon_8_gen3", 0.10f, PriceTier::kPremium},
        {"snapdragon_8_gen2", 0.12f, PriceTier::kFlagship},
        {"snapdragon_8_gen1", 0.08f, PriceTier::kFlagship},
        {"dimensity_9300", 0.06f, PriceTier::kFlagship},
        {"snapdragon_7_gen1", 0.12f, PriceTier::kMidRange},
        {"dimensity_8300", 0.10f, PriceTier::kMidRange},
        {"dimensity_7200", 0.10f, PriceTier::kMidRange},
        {"snapdragon_695", 0.10f, PriceTier::kBudget},
        {"dimensity_6080", 0.10f, PriceTier::kBudget},
        {"helio_g99", 0.12f, PriceTier::kBudget},
    };

    t.budget_screen = {1080, 1080, 2388, 2408, {2.625f}};
    t.mid_screen = {1080, 1080, 2388, 2408, {2.625f, 2.75f}};
    t.flagship_screen = {1080, 1440, 2400, 3200, {2.75f, 3.0f}};

    t.budget_ram = {4096, 6144};
    t.mid_ram = {8192, 12288};
    t.flagship_ram = {12288, 16384};
    t.premium_ram = {16384};

    t.camera_configs = {{1, 2}, {1, 3}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "AK09918 Magnetometer", "AKM"},
    };

    t.build_fp_format = "vivo/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 9. NOTHING (3%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kNothing;
    t.name = "Nothing";
    t.market_weight = 0.03f;

    t.compatible_socs = {
        {"snapdragon_8_gen2", 0.15f, PriceTier::kFlagship},
        {"snapdragon_7_plus_gen2", 0.15f, PriceTier::kMidRange},
        {"dimensity_7200", 0.20f, PriceTier::kMidRange},
        {"dimensity_7050", 0.15f, PriceTier::kMidRange},
        {"dimensity_6080", 0.15f, PriceTier::kBudget},
        {"dimensity_6020", 0.20f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 1080, 1600, 2400, {2.0f, 2.625f}};
    t.mid_screen = {1080, 1080, 2400, 2412, {2.625f}};
    t.flagship_screen = {1080, 1080, 2412, 2412, {2.75f}};

    t.budget_ram = {4096, 6144};
    t.mid_ram = {8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288};

    t.camera_configs = {{1, 2}, {1, 3}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "AK09918 Magnetometer", "AKM"},
    };

    t.build_fp_format =
        "Nothing/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 10. GOOGLE (3%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kGoogle;
    t.name = "Google";
    t.market_weight = 0.03f;

    // Google ONLY uses Tensor chips
    t.compatible_socs = {
        {"tensor_g4", 0.30f, PriceTier::kPremium},
        {"tensor_g3", 0.40f, PriceTier::kFlagship},
        {"tensor_g2", 0.30f, PriceTier::kFlagship},
    };

    t.budget_screen = {1080, 1080, 2340, 2400, {2.625f}};
    t.mid_screen = {1080, 1080, 2400, 2400, {2.625f}};
    t.flagship_screen = {1080, 1440, 2400, 3120, {2.75f, 3.0f}};

    t.budget_ram = {8192};
    t.mid_ram = {8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {12288};

    t.camera_configs = {{1, 2}, {1, 3}};

    t.custom_fonts = {
        "GoogleSans", "ProductSans", "NotoSerif",
        "CutiveMono", "DancingScript", "CarroisGothicSC",
        "ComingSoon", "NotoColorEmoji"};

    t.system_ui_font = "GoogleSans";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"BMI260 Accelerometer", "Bosch",
         "BMI260 Gyroscope", "Bosch",
         "MMC5633 Magnetometer", "MEMSIC"},
        {"LSM6DSR Accelerometer", "STMicroelectronics",
         "LSM6DSR Gyroscope", "STMicroelectronics",
         "MMC5633 Magnetometer", "MEMSIC"},
    };

    t.build_fp_format =
        "google/%c/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  // ============================
  // 11. TECNO (3%)
  // ============================
  {
    ManufacturerTraits t;
    t.id = Manufacturer::kTecno;
    t.name = "Tecno";
    t.market_weight = 0.03f;

    t.compatible_socs = {
        {"dimensity_8200", 0.05f, PriceTier::kFlagship},
        {"dimensity_7050", 0.10f, PriceTier::kMidRange},
        {"dimensity_6080", 0.15f, PriceTier::kBudget},
        {"dimensity_6020", 0.15f, PriceTier::kBudget},
        {"helio_g99", 0.15f, PriceTier::kBudget},
        {"helio_g85", 0.20f, PriceTier::kBudget},
        {"helio_g36", 0.20f, PriceTier::kBudget},
    };

    t.budget_screen = {720, 720, 1600, 1612, {2.0f}};
    t.mid_screen = {1080, 1080, 2340, 2400, {2.625f}};
    t.flagship_screen = {1080, 1080, 2400, 2400, {2.75f}};

    t.budget_ram = {3072, 4096};
    t.mid_ram = {6144, 8192};
    t.flagship_ram = {8192, 12288};
    t.premium_ram = {8192};

    t.camera_configs = {{1, 2}, {1, 3}};

    t.custom_fonts = {"NotoColorEmoji"};
    t.system_ui_font = "Roboto";
    t.emoji_font = "NotoColorEmoji";

    t.sensor_sets = {
        {"SC7A20 Accelerometer", "Sensortek",
         "", "",  // Budget Tecno usually no gyro
         "MMC5603 Magnetometer", "MEMSIC"},
        {"BMA253 Accelerometer", "Bosch",
         "", "",
         "MMC5603 Magnetometer", "MEMSIC"},
    };

    t.build_fp_format =
        "TECNO/%N/%c:%v/%b/%D:user/release-keys";
    t.build_display_format = "%b";

    db.push_back(std::move(t));
  }

  return db;
}

}  // namespace

const std::vector<ManufacturerTraits>& GetAllManufacturerTraits() {
  static const std::vector<ManufacturerTraits> kDatabase = BuildDatabase();
  return kDatabase;
}

const ManufacturerTraits& GetManufacturerTraits(Manufacturer m) {
  const auto& db = GetAllManufacturerTraits();
  int idx = static_cast<int>(m);
  if (idx >= 0 && idx < static_cast<int>(db.size())) {
    return db[idx];
  }
  return db[0];  // Fallback to Samsung
}

Manufacturer PickRandomManufacturer(uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float roll = dist(rng);

  const auto& db = GetAllManufacturerTraits();
  float cumulative = 0.0f;
  for (const auto& traits : db) {
    cumulative += traits.market_weight;
    if (roll <= cumulative) {
      return traits.id;
    }
  }
  return Manufacturer::kSamsung;  // Fallback
}

PriceTier PickRandomPriceTier(uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float roll = dist(rng);

  // Budget 40%, Mid 35%, Flagship 15%, Premium 10%
  if (roll < 0.40f) return PriceTier::kBudget;
  if (roll < 0.75f) return PriceTier::kMidRange;
  if (roll < 0.90f) return PriceTier::kFlagship;
  return PriceTier::kPremium;
}

}  // namespace normal_browser
