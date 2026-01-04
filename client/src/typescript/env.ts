interface Globals {
    url_string: {
        scheme: ("http" | "https")
        port: (number | undefined)
        top_level_domain: string
        second_level_domain: string
        subdomain: string
    },
    websockets: {
        scheme: ("ws" | "wss")
    }
}

export const globals: Readonly<Globals> = {
    url_string: {
        scheme: "http",
        subdomain: "localhost",
        top_level_domain: "",
        second_level_domain: "",
        port: 8080
    },
    websockets: {
        scheme: "ws"
    }
};

export function construct_path(subdir: string): string {
    const gl = globals.url_string;
    const full_url = `${gl.scheme}://${gl.subdomain}${gl.port !== undefined ? ':' + gl.port : ''}/${subdir}`;
    return full_url;
}