<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" encoding="UTF-8"/>
  <xsl:template match="/report">
    <html lang="ja">
      <head><title><xsl:value-of select="@title"/></title></head>
      <body style="margin:22px; color:#20334a; background-color:#f5f7fa; font-family:sans-serif">
        <h1 style="font-size:24px"><xsl:value-of select="@title"/></h1>
        <xsl:apply-templates select="box | review"/>
      </body>
    </html>
  </xsl:template>
  <xsl:template match="box | review">
    <xsl:variable name="flagged" select="self::review or @mark='review'"/>
    <xsl:variable name="background">
      <xsl:choose>
        <xsl:when test="$flagged">#fff1c2</xsl:when>
        <xsl:otherwise>#ffffff</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <div data-title="{@title}" data-reason="{@reason}" style="margin:0 0 12px; padding:12px; border:1px solid #bbc7cf; background-color:{$background}">
      <xsl:if test="$flagged"><xsl:attribute name="class">review</xsl:attribute></xsl:if>
      <h2 style="margin:0; font-size:17px"><xsl:value-of select="@title"/></h2>
      <p style="margin:6px 0 0"><xsl:value-of select="."/></p>
    </div>
  </xsl:template>
</xsl:stylesheet>
