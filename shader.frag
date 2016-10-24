#version 150

in vec3 Colour;
in vec2 Texcoord;

//frag position and normals in view space
in vec4 fragNormalView;
in vec4 fragPositionView;

out vec4 outColor;

uniform sampler2D tex;
const int lightNum = 2;
uniform vec4 light_position[lightNum];
uniform vec4 light_colour[lightNum];
uniform int mode;

void main()
{	
	vec4 diffuse  = vec4(0,0,0,1);
	vec4 specular = vec4(0,0,0,1);
	float d = 0;
	for(int i = 0; i < lightNum; i++)
	{
		//Compute direction of light from frag position in view space
		vec3 lightDisplacement = light_position[i].xyz - fragPositionView.xyz;
	
		//Distance to light
		float d = length(lightDisplacement);
	
		//Normalised light vector
		vec3 normalisedLightDisp = normalize(lightDisplacement);
	
		//Lambertian light equation for diffuse component
		diffuse += dot(normalisedLightDisp, fragNormalView.xyz) * (1/(d*d)) * light_colour[i];
	
		//Compute light reflection using (negate normalisedLightDisp because the light comes from the light to the surface)
		vec3 reflection = reflect(-normalisedLightDisp, fragNormalView.xyz);
	
		//Specular component.
		float shininess = 100;
		float specular_intensity = clamp(dot(reflection, -normalize(fragPositionView.xyz)),0,1);
		specular += pow(specular_intensity, shininess) * (1/(d*d)) * light_colour[i];
	}


	//Ambient
	vec4 ambient = vec4(0.1,0.1,0.1,0.01);

	switch (mode)
	{
		case 0:
			outColor =  ((diffuse + specular) + ambient) * texture(tex, Texcoord) * vec4(Colour, 1.0); //Regular
			break;
		case 1:
			outColor =  (diffuse + specular)  + ambient; //Light only
			break;
		case 2:
			outColor = ambient; //Distance from light
			break;
		case 3:
			outColor = texture(tex,Texcoord) * vec4(Colour, 1.0); //Texture and colour only

	}

    //Final total colour including diffuse, specular, ambient, falloff (with 1/d^2), texture and colour
    //outColor =  ((diffuse + specular) * (1/(d*d)) + ambient) * texture(tex, Texcoord) * vec4(Colour, 1.0);

    //Try these outColours to see just the single components of the light!
	// outColor = diffuse * (1/(d*d));
    // outColor = specular;
    // outColor = specular + diffuse;
    // outColor = (diffuse + specular) * (1/(d*d));
}

