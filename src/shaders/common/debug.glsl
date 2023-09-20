
/**
* Apply color layer for each cascade index (out of 4)
*/
void debug_cascades(in bool colorCascades, in uint cascadeIndex, inout vec3 color) {
    if (colorCascades) {
        switch(cascadeIndex) {
            case 0 :
            color.rgb *= vec3(1.0f, 0.25f, 0.25f); // red
            break;
            case 1 :
            color.rgb *= vec3(0.25f, 1.0f, 0.25f); // green
            break;
            case 2 :
            color.rgb *= vec3(0.25f, 0.25f, 1.0f); // blue
            break;
            case 3 :
            color.rgb *= vec3(1.0f, 1.0f, 0.25f);
            break;
            default:
            color.rgb *= vec3(1.0f, 1.0f, 1.0f) / vec3(cascadeIndex);
            break;
        }
    }
}
