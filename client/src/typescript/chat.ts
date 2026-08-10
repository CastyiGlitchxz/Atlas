export function isHyperlink(src: string): boolean {
    const check = src.match(/(https|http)?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)/);
    return check ? true : false;
};