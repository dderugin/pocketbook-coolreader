# Direct nginx hosting

The primary site is **https://coolreader.nimbushub.eu/**. nginx serves the page
and release files directly from disk. Kubernetes is not involved.

## Layout

- `/var/www/coolreader/releases/<release>/`: website files and `downloads/`.
- `/var/www/coolreader/current`: symlink to the active release.
- `/etc/nginx/sites-available/coolreader.nimbushub.eu`: the virtual host.
- `/etc/nginx/snippets/coolreader-static.conf`: shared static/download rules.
- `/etc/letsencrypt/live/coolreader.nimbushub.eu/`: server-managed TLS material.
- `/var/log/nginx/coolreader.{access,error}.log`: site logs.

Keep TLS keys, Certbot accounts and server logs out of Git and the web root.
The existing virtual hosts can continue to use their own routing.

## Publish content

Prepare a release directory containing `docs/index.html`, `installation.html`, `styles.css`,
`assets/`, `LICENSE.txt`, `release.json` and a `downloads/` directory containing
the eight files from GitHub release `touch-hd3-fw6-r5`. Do not copy the SDK,
firmware image, workspace, SSH keys or nginx configuration into the web root.

On the server, verify `downloads/SHA256SUMS.txt` with `sha256sum -c` from the
downloads directory. The install ZIP must retain this SHA-256:

```text
75b792ddacc3b86c375c988dc6b461ae5cf825c2075adc2cfea98b2f127fe345
```

Use root-owned directories with mode 755 and files with mode 644. Publish into
a new `/var/www/coolreader/releases/<release>` directory, then switch `current`
atomically. For an update, replace `RELEASE_NAME` with the verified new directory name:

```sh
new_release=/var/www/coolreader/releases/RELEASE_NAME
sudo ln -s "$new_release" /var/www/coolreader/current.next
sudo mv -Tf /var/www/coolreader/current.next /var/www/coolreader/current
```

The HTML uses the primary domain for downloads; keep `docs/release.json` and
those links aligned when changing a release. The GitHub release remains a
backup source. The website ZIP within R5 is a historical release snapshot.

## First nginx setup

The supplied TLS configuration assumes an nginx installation already using
Certbot's `/etc/letsencrypt/options-ssl-nginx.conf` and `ssl-dhparams.pem`.

1. Install `coolreader-static.conf` in `/etc/nginx/snippets/`.
2. Ensure `server_names_hash_bucket_size 64;` is configured in the `http`
   context. `coolreader-server-names.conf` can be installed in
   `/etc/nginx/conf.d/` if that directive is not already set elsewhere.
3. Install `coolreader-http-bootstrap.conf` as the new site and enable its
   symlink in `/etc/nginx/sites-enabled/`. Run `sudo nginx -t` before reload.
4. Verify `/.well-known/acme-challenge/` serves files from
   `/var/www/letsencrypt`, and that public DNS and port 80 reach this host.
5. Request a certificate using the existing Certbot account:

```sh
sudo certbot certonly --webroot -w /var/www/letsencrypt \
  --cert-name coolreader.nimbushub.eu -d coolreader.nimbushub.eu \
  --non-interactive --deploy-hook 'nginx -t && systemctl reload nginx'
```

6. Replace the bootstrap virtual host with `coolreader.nimbushub.eu.conf`,
   validate with `sudo nginx -t`, then reload. HTTP now redirects to HTTPS,
   while ACME challenges remain accessible over HTTP.
7. Install the service and timer from this directory in `/etc/systemd/system/`,
   run `sudo systemctl daemon-reload`, then
   `sudo systemctl enable --now coolreader-cert-renew.timer`.

The timer checks this site's certificate twice daily with a randomized delay.
Its stored Certbot deploy hook validates nginx and reloads the new certificate.

## Validate and roll back

Check trusted HTTPS, HTTP-to-HTTPS redirection, the page and all download hashes.
Verify a Range request returns 206, allowing interrupted downloads to resume.
Use `certbot renew --cert-name coolreader.nimbushub.eu --dry-run` to test renewal.

From a LAN without NAT loopback, use split DNS or curl's `--resolve` option to
map the public hostname to the nginx server's LAN address. This keeps normal
hostname and certificate validation enabled; do not disable TLS verification.

To roll back content, switch `current` to the previous release directory with
the same atomic symlink operation. To disable only this site, remove its
`sites-enabled` symlink, run `nginx -t`, then reload nginx. Retain the previous
release and the server's certificates for recovery.
