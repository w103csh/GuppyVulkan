
#version 450
#extension GL_ARB_separate_shader_objects : enable

// GLOBAL
vec3    Ka,     // ambient coefficient
        Kd,     // diffuse coefficient
        Ks,     // specular coefficient
        n,      // normal
        v;      // direction to the camera

vec3 blinnPhongPositional(
    const in vec3 La,           // ambient light intesity
    const in vec3 L,            // diffuse and specular light intensity
    const in vec3 s,            // direction to the light
    const in float shininess    // material shininess
) {
    vec3 ambient = La * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    float sDotN = max(dot(s, n), 0.0);
    // Diffuse color depends on "s" and the "n" only (Lambertian)
    diff = Kd * sDotN;

    // Only calculate specular if the cosine is positive
    if(sDotN > 0.0) {
        // "h" is the halfway vector between "v" & "s" (Blinn)
        vec3 h = normalize(v + s);
        spec = Ks *
                pow(max(dot(h, n), 0.0), shininess);
    }

    return ambient + L * (diff + spec);
}

vec3 blinnPhongSpot(
    const in vec3 La,           // ambient light intesity
    const in vec3 L,            // diffuse and specular light intensity
    const in vec3 s,            // direction to the light
    const in float shininess,   // material shininess
    const in vec3 lightDir,     // direction of the light
    const in float cutoff,      // angle of the light cutoff
    const in float exponent     // 
) {
    vec3 ambient = La * Ka;
    vec3 diff = vec3(0.0), spec = vec3(0.0);

    float cosAng = dot(-s, lightDir);
    float angle = acos(cosAng);

    float spotScale = 0.0;
    if(angle < cutoff) {
        spotScale = pow(cosAng, exponent);
        float sDotN = max(dot(s, n), 0.0);
        diff = Kd * sDotN;
        if (sDotN > 0.0) {
            vec3 h = normalize(v + s);
            spec = Ks * 
                pow(max(dot(h, n), 0.0), shininess);
        }
    }
    return ambient + spotScale * L * (diff + spec);
}