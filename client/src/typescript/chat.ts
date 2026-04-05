export function isHyperlink(src: string): boolean {
    const check = src.match(/^(https|http)?:\/\/[a-zA-Z0-9.-]+(?:(?::[0-9]+)|\.(com|net|jpg|png|jpeg)\\[a-zA-Z0-9.-]+)/);
    return check ? true : false;
};

export function isSameChat(messageSID: string, sid: string): boolean {
    if (messageSID !== sid) return false;
    return true;
};