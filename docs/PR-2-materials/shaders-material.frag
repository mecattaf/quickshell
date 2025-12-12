#version 440

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 tintColor;
    vec4 params;    // x=cornerRadius, y=materialLevel, z=opacity, w=unused
    vec2 size;      // width, height in pixels
    vec2 padding;
};

// Signed distance function for a rounded rectangle
// p: point in normalized [-1,1] space
// b: half-sizes (1,1 for full rect)
// r: corner radius in normalized space
float sdRoundedRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    // Convert from [0,1] to [-1,1] for SDF calculation
    vec2 p = v_texCoord * 2.0 - 1.0;
    
    // Calculate aspect ratio for correct corner rendering
    float aspectRatio = size.x / size.y;
    vec2 adjustedP = p;
    if (aspectRatio > 1.0) {
        adjustedP.x *= aspectRatio;
    } else {
        adjustedP.y /= aspectRatio;
    }
    
    // Adjust bounds for aspect ratio
    vec2 bounds = vec2(1.0);
    if (aspectRatio > 1.0) {
        bounds.x = aspectRatio;
    } else {
        bounds.y = 1.0 / aspectRatio;
    }
    
    // Corner radius from params (already normalized in C++)
    float cornerRadius = params.x;
    
    // Calculate SDF distance
    float d = sdRoundedRect(adjustedP, bounds, cornerRadius);
    
    // Anti-aliased edge (smooth transition over ~2 pixels)
    // The smoothstep range is adjusted based on the size for consistent AA
    float pixelWidth = 2.0 / min(size.x, size.y);
    float alpha = 1.0 - smoothstep(-pixelWidth, pixelWidth, d);
    
    // Apply material level to modify the appearance
    // Higher levels = more opacity/intensity
    float materialMod = 0.5 + (params.y - 1.0) * 0.125; // Maps level 1-5 to 0.5-1.0
    
    // Final color with opacity
    float finalAlpha = tintColor.a * alpha * params.z * materialMod;
    
    // Output premultiplied alpha (tintColor is already premultiplied in C++)
    fragColor = vec4(tintColor.rgb * alpha * params.z * materialMod, finalAlpha);
}
