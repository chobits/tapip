unsigned short chksum(unsigned short *data, int size)
{
	unsigned int sum = 0;
	while (size > 1) {
		sum += *data++;
		size -= 2;
	}

	/* FIXME: Do we handle odd byte right? */
	if (size == 1)
		sum += (*(unsigned char *)data) & 0xff;
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return sum & 0xffff;
}
