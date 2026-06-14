import matrix from "../../test/fixtures/profile-test-matrix.json";

export interface ProfileMatrixVariant {
  pio_env: string;
  pack_id: string;
  role: "meter_router" | "meter_gateway" | "full_router";
  product_profile: string;
  meters: string[];
  board_profile: string;
  caps: {
    surplus_regulation: boolean;
    triac: boolean;
    multi_action: boolean;
    source_test_inject: boolean;
  };
  hil_suite: string;
  ui: {
    single_meter: boolean;
    show_measurement_source: boolean;
    show_advanced_meter_sources: boolean;
    show_follow_triac_pwm_option: boolean;
  };
}

export const profileMatrixVariants = matrix.variants as ProfileMatrixVariant[];

export function profileVariantByPioEnv(pioEnv: string): ProfileMatrixVariant | undefined {
  return profileMatrixVariants.find((v) => v.pio_env === pioEnv);
}

export function profileVariantByProductProfile(
  productProfile: string,
): ProfileMatrixVariant | undefined {
  return profileMatrixVariants.find((v) => v.product_profile === productProfile);
}
