/*
 * Driver for the ESS SABRE9018Q2C
 *
 * Author: Satoru Kawase, Takahito Nishiara
 *      Copyright 2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>

#include "es9018k2m.h"

#define SABRE9018C2M_REG_64 0x40 // Register #64

struct es9018k2m_priv {
    struct regmap *regmap;
    unsigned int fmt;
    uint8_t volume1;
    uint8_t volume2;
    bool is_muted;
};

uint8_t SABRE9018Q2C_VOLUME1;
uint8_t SABRE9018Q2C_VOLUME2;
bool SABRE9018Q2C_isMuted;
/* SABRE9018Q2C Default Register Value */
static const struct reg_default es9018k2m_reg_defaults[] = {
	{ 0, 0x00 },
	{ 1, 0x8c },
	{ 4, 0x00 },
	{ 5, 0x68 },
	{ 6, 0x42 },
	{ 7, 0x80 },
	{ 8, 0x10 },
	{ 9, 0x00 },
	{ 10,0x00 },
	{ 11,0x02 },
	{ 12,0x5a },
	{ 13,0x40 },
	{ 14,0x8a },
	{ 15,0x80 },
	{ 16,0x80 },
	{ 17,0xff },
	{ 18,0xff },
	{ 19,0xff },
	{ 20,0x7f },
	{ 21,0x00 },
	{ 26,0x00 },
	{ 27,0x00 },
	{ 28,0x00 },
	{ 29,0x00 },
	{ 30,0x00 },
};


static bool es9018k2m_writeable(struct device *dev, unsigned int reg)
{
	if(reg > ES9018K2M_CACHEREGNUM)
		return  0;
	else if(reg == 0x2 || reg == 0x3)
		return 0;
	else
		return 1;
}

static bool es9018k2m_readable(struct device *dev, unsigned int reg)
{
	if(reg <= ES9018K2M_CACHEREGNUM && reg != 2 && reg !=3)
		return 1;
	else if(65 <= reg && reg <= 69)
		return 1;
	else if(70 <= reg && reg <= 93)
		return 1;
	else
		return 0;
}

static bool es9018k2m_volatile(struct device *dev, unsigned int reg)
{
	return false;
}

static int es9018k2m_mute(struct snd_soc_dai *dai, int mute)
{
    struct snd_soc_component *component = dai->component;
    struct es9018k2m_priv *es9018k2m = snd_soc_component_get_drvdata(component);
    int ret, val;

    if (!es9018k2m) {
        dev_err(component->dev, "Failed to get driver data\n");
        return -EINVAL;
    }

    if (mute) {
        if (!es9018k2m->is_muted) {
            val = snd_soc_component_read(component, ES9018K2M_VOLUME1);
            if (val < 0) return val;
            es9018k2m->volume1 = val;

            val = snd_soc_component_read(component, ES9018K2M_VOLUME2);
            if (val < 0) return val;
            es9018k2m->volume2 = val;

            es9018k2m->is_muted = true;
        }
        ret = snd_soc_component_write(component, ES9018K2M_VOLUME1, 0xFF);
        if (ret < 0) return ret;

        ret = snd_soc_component_write(component, ES9018K2M_VOLUME2, 0xFF);
        if (ret < 0) return ret;
    } else {
        ret = snd_soc_component_write(component, ES9018K2M_VOLUME1, es9018k2m->volume1);
        if (ret < 0) return ret;

        ret = snd_soc_component_write(component, ES9018K2M_VOLUME2, es9018k2m->volume2);
        if (ret < 0) return ret;

        es9018k2m->is_muted = false;
    }
    return 0;
}

static int es9018k2m_unmute(struct snd_soc_dai *dai)
{
    struct snd_soc_component *component = dai->component;
    struct es9018k2m_priv *es9018k2m = snd_soc_component_get_drvdata(component);
    int ret;

    if (!es9018k2m) {
        dev_err(component->dev, "Failed to get driver data\n");
        return -EINVAL;
    }

    ret = snd_soc_component_write(component, ES9018K2M_VOLUME1, es9018k2m->volume1);
    if (ret < 0) return ret;

    ret = snd_soc_component_write(component, ES9018K2M_VOLUME2, es9018k2m->volume2);
    if (ret < 0) return ret;

    es9018k2m->is_muted = false;
    return 0;
}

/* Volume Scale */
static const DECLARE_TLV_DB_SCALE(volume_tlv, -12750, 50, 1);

/* Control */
static const struct snd_kcontrol_new es9018k2m_controls[] = {
SOC_DOUBLE_R_TLV("Digital Playback Volume", ES9018K2M_VOLUME1, ES9018K2M_VOLUME2,
		 0, 255, 1, volume_tlv),
};


static const uint32_t es9018k2m_dai_rates_master[] = {
	44100, 48000, 88200, 96000, 176400, 192000
};

static const struct snd_pcm_hw_constraint_list constraints_master = {
	.list  = es9018k2m_dai_rates_master,
	.count = ARRAY_SIZE(es9018k2m_dai_rates_master),
};

static const uint32_t es9018k2m_dai_rates_slave[] = {
	8000, 11025, 16000, 22050, 32000,
	44100, 48000, 64000, 88200, 96000, 176400, 192000, 352800, 384000
};

static const struct snd_pcm_hw_constraint_list constraints_slave = {
	.list  = es9018k2m_dai_rates_slave,
	.count = ARRAY_SIZE(es9018k2m_dai_rates_slave),
};

static int es9018k2m_dai_startup_master(
        struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    //printk(KERN_INFO "Message: in es9018k2m_dai_startup_master");
    struct snd_soc_component *component = dai->component;
    int ret;

    ret = snd_pcm_hw_constraint_list(substream->runtime,
                    0, SNDRV_PCM_HW_PARAM_RATE, &constraints_master);
    if (ret != 0) {
        dev_err(component->dev, "Failed to setup rates constraints: %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_mask64(substream->runtime,
                    SNDRV_PCM_HW_PARAM_FORMAT, SNDRV_PCM_FMTBIT_S32_LE);
    if (ret != 0) {
        dev_err(component->dev, "Failed to setup format constraints: %d\n", ret);
    }

    return ret;
}

static int es9018k2m_dai_startup_slave(
        struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    struct snd_soc_component *component = dai->component;
    int ret;

    ret = snd_pcm_hw_constraint_list(substream->runtime,
                    0, SNDRV_PCM_HW_PARAM_RATE, &constraints_slave);
    if (ret != 0) {
        dev_err(component->dev, "Failed to setup rates constraints: %d\n", ret);
    }

    return ret;
}

static int es9018k2m_dai_startup(
                struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	//printk(KERN_INFO "Message es9018k2m_dai_startup");
    struct snd_soc_component *component = dai->component;
    struct es9018k2m_priv *es9018k2m = snd_soc_component_get_drvdata(component);

    if (!es9018k2m) {
        dev_err(component->dev, "Failed to get driver data\n");
        return -EINVAL;
    }

    es9018k2m_mute(dai, 1);

    switch (es9018k2m->fmt & SND_SOC_DAIFMT_MASTER_MASK) {
    case SND_SOC_DAIFMT_CBM_CFM:
        return es9018k2m_dai_startup_master(substream, dai);

    case SND_SOC_DAIFMT_CBS_CFS:
        return es9018k2m_dai_startup_slave(substream, dai);

    default:
        return -EINVAL;
    }
}

static int es9018k2m_hw_params(
        struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,
        struct snd_soc_dai *dai)
{
    struct snd_soc_component *component = dai->component;
    int ret, iface;

    ret = snd_soc_component_read(component, ES9018K2M_INPUT_CONFIG);
    if (ret < 0)
        return ret;

    iface = ret & 0x3f;

    switch (params_format(params)) {
        case SNDRV_PCM_FORMAT_S16_LE:
            iface |= 0x0;
            break;
        case SNDRV_PCM_FORMAT_S24_LE:
            iface |= 0x80;
            break;
        case SNDRV_PCM_FORMAT_S32_LE:
            iface |= 0x80;
            break;
        default:
            return -EINVAL;
    }

    ret = snd_soc_component_write(component, ES9018K2M_INPUT_CONFIG, iface);
    if (ret < 0)
        return ret;

    return 0;
}

static int es9018k2m_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    struct snd_soc_component *component = dai->component;
    struct es9018k2m_priv *es9018k2m = snd_soc_component_get_drvdata(component);

    if (!es9018k2m) {
        dev_err(component->dev, "Failed to get driver data\n");
        return -EINVAL;
    }

    /* interface format */
    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
    case SND_SOC_DAIFMT_I2S:
        break;

    case SND_SOC_DAIFMT_RIGHT_J:
    case SND_SOC_DAIFMT_LEFT_J:
    default:
        return -EINVAL;
    }

    /* clock inversion */
    if ((fmt & SND_SOC_DAIFMT_INV_MASK) != SND_SOC_DAIFMT_NB_NF) {
        return -EINVAL;
    }

    /* Set Audio Data Format */
    es9018k2m->fmt = fmt;

    return 0;
}


static void es9018k2m_shutdown(struct snd_pcm_substream * substream, struct snd_soc_dai *dai)
{
	es9018k2m_mute(dai, 1);
}

static int es9018k2m_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	switch(cmd)
	{
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			mdelay(1500);
			es9018k2m_unmute(dai);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			es9018k2m_mute(dai, 1);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static const struct snd_soc_dai_ops es9018k2m_dai_ops = {
    .startup   = es9018k2m_dai_startup,
    .hw_params = es9018k2m_hw_params,
    .set_fmt   = es9018k2m_set_fmt,
    .shutdown  = es9018k2m_shutdown,
    .trigger   = es9018k2m_dai_trigger,
};


static struct snd_soc_dai_driver es9018k2m_dai = {
	.name = "es9018k2m-dai",
	.playback = {
		.stream_name  = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min = 8000,
		.rate_max = 384000,
		.formats      = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | \
		    SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &es9018k2m_dai_ops,
};

static struct snd_soc_component_driver es9018k2m_component_driver = {
    .controls         = es9018k2m_controls,
    .num_controls     = ARRAY_SIZE(es9018k2m_controls),
};



static const struct regmap_config es9018k2m_regmap = {
	.reg_bits         = 8,
	.val_bits         = 8,
	.max_register     = 93,

	.reg_defaults     = es9018k2m_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(es9018k2m_reg_defaults),

	.writeable_reg    = es9018k2m_writeable,
	.readable_reg     = es9018k2m_readable,
	.volatile_reg     = es9018k2m_volatile,

	.cache_type       = REGCACHE_RBTREE,
};

bool es9018k2m_check_chip_id(struct snd_soc_component *component)
{
	//printk(KERN_INFO "Message es9018k2m_check_chip_id");
    int ret, chip_id;

    ret = snd_soc_component_read(component, SABRE9018C2M_REG_64);
    if (ret < 0) {
	    //printk(KERN_INFO "Failed to read chip ID\n");
        dev_err(component->dev, "Failed to read chip ID\n");
        return false;
    }

    chip_id = (ret >> 2) & 0x07; // Extracting bits [4:2] for chip_id

    if (chip_id == 0) { // 3’d0 corresponds to SABRE9018C2M
        return true;
    } else {
        return false;
    }
}

EXPORT_SYMBOL_GPL(es9018k2m_check_chip_id);

static int es9018k2m_probe(struct device *dev, struct regmap *regmap)
{
	//printk(KERN_INFO "Message es9018k2m_probe");
    struct es9018k2m_priv *es9018k2m;
    int ret;

    es9018k2m = devm_kzalloc(dev, sizeof(*es9018k2m), GFP_KERNEL);
    if (!es9018k2m) {
        dev_err(dev, "devm_kzalloc failed");
        return -ENOMEM;
    }

    es9018k2m->regmap = regmap;
    dev_set_drvdata(dev, es9018k2m);

    ret = snd_soc_register_component(dev, &es9018k2m_component_driver,
                                     &es9018k2m_dai, 1);
    if (ret != 0) {
        dev_err(dev, "Failed to register component: %d\n", ret);
        return ret;
    }

    return 0;
}

static void es9018k2m_remove(struct device *dev)
{
    snd_soc_unregister_component(dev);
}



static int es9018k2m_i2c_probe(
		struct i2c_client *i2c, const struct i2c_device_id *id)
{
	//printk(KERN_INFO "Message es9018k2m_i2c_probe");
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(i2c, &es9018k2m_regmap);
	if (IS_ERR(regmap)) {
		return PTR_ERR(regmap);
	}

	return es9018k2m_probe(&i2c->dev, regmap);
}

static void es9018k2m_i2c_remove(struct i2c_client *i2c)
{
	es9018k2m_remove(&i2c->dev);
}


static const struct i2c_device_id es9018k2m_i2c_id[] = {
	{ "es9018k2m", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, es9018k2m_i2c_id);

static const struct of_device_id es9018k2m_of_match[] = {
	{ .compatible = "ess,es9018k2m", },
	{ }
};
MODULE_DEVICE_TABLE(of, es9018k2m_of_match);
static struct i2c_driver es9018k2m_i2c_driver = {
    .driver = {
        .name = "es9018k2m-i2c",
        .of_match_table = of_match_ptr(es9018k2m_of_match),
    },
    .probe = es9018k2m_i2c_probe,
    .remove = es9018k2m_i2c_remove,
    .id_table = es9018k2m_i2c_id,
};
module_i2c_driver(es9018k2m_i2c_driver);


MODULE_DESCRIPTION("AIODE DAC II codec driver");
MODULE_AUTHOR("Simon B <simonb@kaizo.org>, Satoru Kawase <satoru.kawase@gmail.com>");
MODULE_LICENSE("GPL");
