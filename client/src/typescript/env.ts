interface Globals {
    url_string: {
        scheme: string
        port: (number | undefined)
        top_level_domain: string
        second_level_domain: string
        subdomain: string
    },
    websockets: {
        scheme: string
    }
}

export const globals: Readonly<Globals> = {
    url_string: {
        scheme: process.env.NEXT_PUBLIC_SCHEME,
        subdomain: process.env.NEXT_PUBLIC_SUBDOMAIN,
        top_level_domain: "",
        second_level_domain: "",
        port: parseInt(process.env.NEXT_PUBLIC_PORT)
    },
    websockets: {
        scheme: process.env.NEXT_PUBLIC_WEBSOCKET_SCHEME
    }
};

export function construct_path(subdir: string): string {
    const gl = globals.url_string;
    const full_url = `${gl.scheme}://${gl.subdomain}${gl.port !== undefined ? ':' + gl.port : ''}/${subdir}`;
    return full_url;
}